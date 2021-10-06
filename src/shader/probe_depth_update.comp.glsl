#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(
  local_size_x = octomap_depth_texel_size.x,
  local_size_y = octomap_depth_texel_size.y,
  local_size_z = 1
) in;

// :ProbeDepthFormat
layout(binding = 0, rg16) uniform image2D probe_depth_map;
layout(binding = 1, rg16) uniform image2D probe_depth_map_previous;

layout(binding = 2) uniform sampler2D zbuffer2_image;
layout(binding = 3) uniform Frame { FrameData data; } frame;

void main() {
  ivec2 octomap_coord = ivec2(gl_LocalInvocationID.xy);
  uvec3 probe_coord = gl_WorkGroupID;

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 texel_coord_base = probe_ray_count_factors * (
    probe_coord.xy +
    z_subcoord * frame.data.probe.grid_size.xy
  );

  const float border = 1.0;
  vec2 unique_texel_size = octomap_depth_texel_size - 2.0 * border;
  vec3 octomap_direction = octo_decode(
    mod(octomap_coord - border + 0.5, unique_texel_size) / (0.5 * unique_texel_size) - 1.0
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

      float ray_distance = texture(
        zbuffer2_image,
        (
          (vec2(texel_coord_base + uvec2(x, y)) + 0.5) /
            frame.data.probe.secondary_gbuffer_texel_size
        )
      ).r;

      float weight = max(0.0, dot(octomap_direction, ray_direction));

      float max_distance = length(frame.data.probe.grid_world_position_delta * 0.5);
      if (ray_distance == 0.0) {
        // no hit
        ray_distance = max_distance;
      }
      ray_distance = min(
        max_distance,
        ray_distance
      );

      weight = pow(weight, frame.data.probe.depth_sharpness);

      value += vec4(
        ray_distance * weight,
        ray_distance * ray_distance * weight,
        0,
        weight
      );
    }
  }
  if (value.w > 0.0) {
    value /= value.w;
  }

  if (frame.data.is_frame_sequential) {
    ivec3 probe_coord_prev = ivec3(probe_coord) + frame.data.probe.change_from_prev;
    // @Think: `+` seems wrong here, instead of `-`, but it actually works.

    // Are vector boolean expressions possible in GLSL? if so, this could use them.
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
      ivec2 texel_coord_prev = octomap_coord + ivec2(octomap_depth_texel_size) * (
        probe_coord_prev.xy +
        ivec2(z_subcoord_prev * frame.data.probe.grid_size.xy)
      );

      vec4 previous = imageLoad(
        probe_depth_map_previous,
        texel_coord_prev
      );

      value = previous * 0.99 + 0.01 * value;
      // @Cleanup :MoveToUniform hysteresis_thing
    }
  }

  ivec2 texel_coord = octomap_coord + ivec2(octomap_depth_texel_size) * (
    ivec2(probe_coord.xy) +
    ivec2(z_subcoord * frame.data.probe.grid_size.xy)
  );

  imageStore(
    probe_depth_map,
    texel_coord,
    value
  );
}