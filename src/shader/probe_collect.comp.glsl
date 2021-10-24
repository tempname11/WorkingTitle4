#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(
  local_size_x = octomap_texel_size.x,
  local_size_y = octomap_texel_size.y,
  local_size_z = 1
) in;

// :ProbeIrradianceFormat
layout(binding = 0, rgba16f) uniform image2D probe_irradiance;
layout(binding = 1, rgba16f) uniform image2D probe_irradiance_previous;

layout(binding = 2) uniform sampler2D probe_radiance;
layout(binding = 3) uniform Frame { FrameData data; } frame;
layout(binding = 4, r32ui) uniform uimage2D probe_attention_prev; // :ProbeAttentionFormat

layout(push_constant) uniform Cascade {
  vec3 world_position_delta;
  uint level;
} cascade;

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

  if (!frame.data.flags.disable_indirect_attention) {
    uint attention = imageLoad(
      probe_attention_prev,
      ivec2(combined_coord)
    ).r;

    // :ProbeWrapping invalidate nodes that just spawned. (set to 0)

    if (attention == 0) {
      if (frame.data.is_frame_sequential) {
        // :ProbeWrapping delete this

        ivec3 probe_coord_prev = (
          ivec3(probe_coord)
          + frame.data.probe.cascades[cascade.level].change_from_prev
        );

        bool out_of_bounds = (false
          || probe_coord_prev.x < 0
          || probe_coord_prev.y < 0
          || probe_coord_prev.z < 0
          || probe_coord_prev.x >= frame.data.probe.grid_size.x
          || probe_coord_prev.y >= frame.data.probe.grid_size.y
          || probe_coord_prev.z >= frame.data.probe.grid_size.z
        );

        if (!out_of_bounds) {
          uvec2 z_subcoord_prev = uvec2(
            probe_coord_prev.z % frame.data.probe.grid_size_z_factors.x,
            probe_coord_prev.z / frame.data.probe.grid_size_z_factors.x
          );

          ivec2 texel_coord_prev = octomap_coord + ivec2(
            octomap_texel_size * (
              probe_coord_prev.xy +
              frame.data.probe.grid_size.xy * (
                z_subcoord_prev +
                frame.data.probe.grid_size_z_factors * (
                  cascade_subcoord
                )
              )
            )
          );

          vec4 previous = imageLoad(
            probe_irradiance_previous,
            texel_coord_prev
          );

          ivec2 texel_coord = octomap_coord + ivec2(
            octomap_texel_size * (
              probe_coord.xy +
              frame.data.probe.grid_size.xy * (
                z_subcoord +
                frame.data.probe.grid_size_z_factors * (
                  cascade_subcoord
                )
              )
            )
          );

          imageStore(
            probe_irradiance,
            texel_coord,
            previous
          );
        }
      }

      return;
    }
  }

  uvec2 texel_coord_base = probe_ray_count_factors * combined_coord;
  vec2 unique_texel_size = octomap_texel_size - 1.0;
  vec3 octomap_direction = octo_decode(
    (mod(octomap_coord - 0.5, unique_texel_size) / unique_texel_size) * 2.0 - 1.0
  );

  vec4 value = vec4(0.0);
  for (uint x = 0; x < probe_ray_count_factors.x; x++) {
    for (uint y = 0; y < probe_ray_count_factors.y; y++) {
      uint ray_index = x + y * probe_ray_count_factors.x;

      vec3 ray_direction = get_probe_ray_direction(
        ray_index,
        probe_ray_count,
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

  if (frame.data.is_frame_sequential) {
    // :ProbeWrapping instead of adding change_from_prev,
    // use it to understand if invalidation is needed

    ivec3 probe_coord_prev = (
      ivec3(probe_coord)
      + frame.data.probe.cascades[cascade.level].change_from_prev
    );
    // @Think: `+` seems wrong here, instead of `-`, but it actually works.

    // @Cleanup: use vector boolean ops?
    bool out_of_bounds = (false
      || probe_coord_prev.x < 0
      || probe_coord_prev.y < 0
      || probe_coord_prev.z < 0
      || probe_coord_prev.x >= frame.data.probe.grid_size.x
      || probe_coord_prev.y >= frame.data.probe.grid_size.y
      || probe_coord_prev.z >= frame.data.probe.grid_size.z
    );

    if (!out_of_bounds) {
      uvec2 z_subcoord_prev = uvec2(
        probe_coord_prev.z % frame.data.probe.grid_size_z_factors.x,
        probe_coord_prev.z / frame.data.probe.grid_size_z_factors.x
      );

      ivec2 texel_coord_prev = octomap_coord + ivec2(
        octomap_texel_size * (
          probe_coord_prev.xy +
          frame.data.probe.grid_size.xy * (
            z_subcoord_prev +
            frame.data.probe.grid_size_z_factors * (
              cascade_subcoord
            )
          )
        )
      );

      vec4 previous = imageLoad(
        probe_irradiance_previous,
        texel_coord_prev
      );

      value = previous * 0.98 + 0.02 * value;
      // @Cleanup :MoveToUniform hysteresis_thing
    }
  }

  ivec2 texel_coord = octomap_coord + ivec2(
    octomap_texel_size * (
      probe_coord.xy +
      frame.data.probe.grid_size.xy * (
        z_subcoord +
        frame.data.probe.grid_size_z_factors * (
          cascade_subcoord
        )
      )
    )
  );

  imageStore(
    probe_irradiance,
    texel_coord,
    value
  );
}