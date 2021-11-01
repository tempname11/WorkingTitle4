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

shared uint shared_error_uint;

layout(push_constant) uniform Cascade {
  uint level;
} cascade;

void main() { 
  if (gl_LocalInvocationIndex == 0) {
    shared_error_uint = 0;
  }

  ivec2 octomap_coord = ivec2(gl_LocalInvocationID.xy);
  uvec4 workset_data = probe_worksets[cascade.level].data[gl_WorkGroupID.x];
  uvec3 probe_coord = workset_data.xyz;
  uint attention = workset_data.w;

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
  uint last_update_frame_number = confidence_packed.r * 65536 + confidence_packed.g;
  uint frames_passed = frame.data.number - last_update_frame_number;
  float volatility = confidence_packed.b / 65535.0;
  float accumulator = confidence_packed.a / 65535.0 * PROBE_ACCUMULATOR_MAX;
  float beta = max(0.0, accumulator - min(frames_passed, PROBE_ACCUMULATOR_MAX));

  float alpha = 1.0 / max(1.0 / PROBE_MAX_SKIPS, volatility);
  vec4 irradiance_write = (
    irradiance_read * beta
    + value * alpha 
  );
  irradiance_write /= beta + alpha;

  imageStore(
    probe_irradiance,
    irradiance_texel_coord,
    irradiance_write
  );

  { // update confidence
    vec3 error_rgb = abs(value.rgb - irradiance_read.rgb);
    float error_linear = max(error_rgb.r, max(error_rgb.g, error_rgb.b));
    float error = clamp(
      error_linear / (frame.data.luminance_moving_average + error_linear), // @Hack
      0.0,
      1.0
    );
    if (value.w == 0.0) { // probe-in-the-wall? check this later.
      error = 0.0;
    }

    barrier(); // @Think: are these barriers overkill or necessary?
    memoryBarrierShared();
    atomicMax(shared_error_uint, uint(error * 65535));
    barrier();
    memoryBarrierShared();

    if (gl_LocalInvocationIndex == 0) {
      float max_error = shared_error_uint / 65535.0;
      volatility = clamp(
        (volatility * 15.0 + max_error * 1.0)
          / (15.0 + 1.0),
        0.0,
        1.0
      ); // pretty sure this needs to be improved a lot.

      imageStore(
        probe_confidence,
        ivec2(combined_coord),
        uvec4(
          frame.data.number / 65536,
          frame.data.number % 65536,
          volatility * 65535.0,
          min(beta + alpha + 1.0, PROBE_ACCUMULATOR_MAX) / PROBE_ACCUMULATOR_MAX * 65535.0
        ) // :ProbeConfidenceFormat
      );
    }
  }
}