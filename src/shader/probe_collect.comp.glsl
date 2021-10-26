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
layout(binding = 2) uniform sampler2D probe_radiance;
layout(binding = 3) uniform Frame { FrameData data; } frame;
layout(binding = 4, r32ui) uniform readonly uimage2D probe_attention_prev; // :ProbeAttentionFormat

layout(push_constant) uniform Cascade {
  vec3 world_position_delta;
  uint level;
} cascade;

const float HYSTERESIS_PER_FRAME = 0.99;

void main() {
  ivec2 octomap_coord = ivec2(gl_LocalInvocationID.xy);
  uvec3 probe_coord = gl_WorkGroupID;

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 cascade_subcoord = uvec2(
    cascade.level % frame.data.probe.cascade_count_factors.x,
    cascade.level / frame.data.probe.cascade_count_factors.x
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

  if (!frame.data.flags.disable_indirect_attention) {
    uint attention = imageLoad(
      probe_attention_prev,
      ivec2(combined_coord)
    ).r;

    if (attention == 0) {
      ivec3 infinite_grid_min = frame.data.probe.cascades[cascade.level].infinite_grid_min;
      ivec3 infinite_grid_min_prev = frame.data.probe.cascades[cascade.level].infinite_grid_min_prev;

      ivec3 infinite_grid_coord = (0
        + (ivec3(probe_coord) - infinite_grid_min) % ivec3(frame.data.probe.grid_size)
        + infinite_grid_min
      );
      bool out_of_bounds = (false
        || any(lessThan(
          infinite_grid_coord,
          infinite_grid_min_prev
        ))
        || any(greaterThan(
          infinite_grid_coord,
          infinite_grid_min_prev + ivec3(frame.data.probe.grid_size) - 1
        ))
      );
      if (out_of_bounds) {
        imageStore(
          probe_irradiance,
          irradiance_texel_coord,
          vec4(0.0)
        );
      }

      return;
    }
  }

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

  if (frame.data.is_frame_sequential) {
    vec4 previous = imageLoad(
      probe_irradiance,
      irradiance_texel_coord
    );

    value = previous * HYSTERESIS_PER_FRAME + (1.0 - HYSTERESIS_PER_FRAME) * value;
  }

  imageStore(
    probe_irradiance,
    irradiance_texel_coord,
    value
  );
}