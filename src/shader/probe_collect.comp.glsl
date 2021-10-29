#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(
  local_size_x = OCTOMAP_TEXEL_SIZE.x,
  local_size_y = OCTOMAP_TEXEL_SIZE.y,
  local_size_z = 1
) in;

layout(binding = 0, rgba16f) uniform image2D probe_irradiance; // :ProbeIrradianceFormat
layout(binding = 1, rgba16ui) uniform uimage2D probe_confidence; // :ProbeConfidenceFormat
layout(binding = 2) uniform sampler2D probe_radiance;
layout(binding = 3) uniform Frame { FrameData data; } frame;
layout(binding = 4) readonly buffer ProbeWorkset {
  uvec4 data[];
} probe_worksets[PROBE_CASCADE_COUNT];

vec3 to_perception_space(vec3 l) {
  return l / (frame.data.luminance_moving_average + l);
}

shared uint overall_surprise_uint;

layout(push_constant) uniform Cascade {
  uint level;
} cascade;

void main() {
  if (gl_LocalInvocationIndex == 0) {
    overall_surprise_uint = 0;
  }

  ivec2 octomap_coord = ivec2(gl_LocalInvocationID.xy);
  uvec3 probe_coord = probe_worksets[cascade.level].data[gl_WorkGroupID.x].xyz;

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 cascade_subcoord = uvec2(
    cascade.level % PROBE_CASCADE_COUNT_FACTORS.x,
    cascade.level / PROBE_CASCADE_COUNT_FACTORS.x
  );

  uvec2 combined_coord = (
    probe_coord.xy +
    frame.data.probe.grid_size.xy * (
      z_subcoord +
      frame.data.probe.grid_size_z_factors * (
        cascade_subcoord
      )
    )
  );

  ivec2 irradiance_texel_coord = octomap_coord + ivec2(
    OCTOMAP_TEXEL_SIZE * combined_coord
  );

  vec4 value = vec4(0.0);
  { // collect radiance
    uvec2 texel_coord_base = PROBE_RAY_COUNT_FACTORS * combined_coord;
    vec2 unique_texel_size = OCTOMAP_TEXEL_SIZE - 1.0;
    vec3 octomap_direction = octo_decode(
      (mod(octomap_coord - 0.5, unique_texel_size) / unique_texel_size) * 2.0 - 1.0
    );

    for (uint x = 0; x < PROBE_RAY_COUNT_FACTORS.x; x++) {
      for (uint y = 0; y < PROBE_RAY_COUNT_FACTORS.y; y++) {
        uint ray_index = x + y * PROBE_RAY_COUNT_FACTORS.x;

        vec3 ray_direction = get_probe_ray_direction(
          ray_index,
          PROBE_RAY_COUNT,
          frame.data.probe.random_orientation
        );

        vec3 ray_radiance = texture(
          probe_radiance,
          (
            (vec2(texel_coord_base + uvec2(x, y)) + 0.5) /
              textureSize(probe_radiance, 0)
          )
        ).rgb;

        float weight = max(0.0, dot(octomap_direction, ray_direction));

        value += vec4(ray_radiance * weight, weight);
      }
    }
    if (value.w > 0.0) {
      value /= value.w;
    }
  }

  vec4 irradiance_read = imageLoad(
    probe_irradiance,
    irradiance_texel_coord
  );

  uvec4 confidence_packed = imageLoad(
    probe_confidence,
    ivec2(combined_coord)
  ); // :ProbeConfidenceFormat
  float confidence = confidence_packed.r / 65535.0;

  vec4 irradiance_write = irradiance_read * confidence + 0.01 * value;
  irradiance_write /= (confidence + 0.01);

  if (frame.data.flags.debug_C) {
    irradiance_write = irradiance_read;
  }

  imageStore(
    probe_irradiance,
    irradiance_texel_coord,
    irradiance_write
  );

  { // update confidence
    float surprise = clamp(
      length(
        to_perception_space(irradiance_read.rgb) -
        to_perception_space(value.rgb)
      ),
      0.0,
      1.0
    );

    barrier(); // @Think: are these barriers overkill or necessary?
    memoryBarrierShared();
    atomicMax(overall_surprise_uint, uint(surprise * 65535));
    barrier();
    memoryBarrierShared();

    if (gl_LocalInvocationIndex == 0) {
      float overall_surprise = overall_surprise_uint / 65535.0;
      confidence = clamp(
        1.0 - (
          (1.0 - confidence) *
          pow(2.0, overall_surprise - 0.25)
        ),
        0.0,
        0.9
      );

      imageStore(
        probe_confidence,
        ivec2(combined_coord),
        uvec4(confidence * 65535, 0.0, 0.0, 0.0) // :ProbeConfidenceFormat
      );
    }
  }
}