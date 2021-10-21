#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/sky.glsl"
#include "common/light_model.glsl"
#include "common/probes.glsl"
#include "common/frame.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 4) uniform sampler2D probe_light_map_previous;
layout(binding = 5) uniform sampler2D probe_depth_map_previous;
layout(binding = 6) uniform Frame { FrameData data; } frame;
layout(binding = 7) uniform writeonly uimage2D probe_attention;
layout(binding = 8, r32ui) uniform uimage2D probe_attention_prev; // :ProbeAttentionFormat

void main() {
  if (!frame.data.is_frame_sequential) {
    result = vec3(0.0); // @Performance: rather, don't call vkCmdDraw at all!
    return;
  }

  // @Cleanup: share this code with other L2 passes, they have a lot in common.
  uvec2 texel_coord = uvec2(
    (position * 0.5 + 0.5) * frame.data.secondary_gbuffer_texel_size
  );
  // Truncated, otherwise would need to subtract 0.5.

  // @CopyPaste :L2_ProbeCoord
  // @Performance: could do bit operations here if working with powers of 2.

  // Ray.
  uvec2 ray_subcoord = texel_coord % probe_ray_count_factors;
  uint ray_index = (
    ray_subcoord.x +
    ray_subcoord.y * probe_ray_count_factors.x
  );
  texel_coord /= probe_ray_count_factors;

  // attention!
  ivec2 combined_coord = ivec2(texel_coord);
  if (!frame.data.flags.disable_indirect_attention) {
    uint attention = imageLoad(
      probe_attention_prev,
      combined_coord
    ).r;

    if (attention == 0) {
      result = vec3(0.0);
      return;
    }
  }

  // Grid XY.
  uvec2 probe_grid_coord_xy = texel_coord % frame.data.probe.grid_size.xy;
  texel_coord /= frame.data.probe.grid_size.xy;

  // Grid Z.
  uvec2 z_subcoord = texel_coord % frame.data.probe.grid_size_z_factors;
  uvec3 probe_grid_coord = uvec3(
    probe_grid_coord_xy,
    (
      z_subcoord.x +
      z_subcoord.y * frame.data.probe.grid_size_z_factors.x
    )
  );
  texel_coord /= frame.data.probe.grid_size_z_factors;

  // Cascade.
  uvec2 cascade_subcoord = texel_coord;
  uint cascade_level = (
    cascade_subcoord.x +
    cascade_subcoord.y * frame.data.probe.cascade_count_factors.x
  );

  vec3 cascade_world_position_delta = (
    frame.data.probe.grid_world_position_delta_c0 *
    pow(2.0, cascade_level) // bit shift maybe?
  );

  // everything is in world space.
  vec3 probe_origin = (
    frame.data.probe.cascades[cascade_level].world_position_zero +
    cascade_world_position_delta * probe_grid_coord
  );
  vec3 probe_raydir = get_probe_ray_direction(
    ray_index,
    probe_ray_count,
    frame.data.probe.random_orientation
  );
  float t = subpassLoad(zchannel).r;
  if (t < 0.0) {
    if (probe_raydir.z < 0.0) { discard; }
    result = sky(
      probe_raydir,
      frame.data.sky_sun_direction,
      frame.data.sky_intensity,
      false /* show_sun */
    );
    if (frame.data.flags.disable_sky) {
      result *= 0.0;
    }
    return;
  } else if (t == 0.0) {
    discard;
  }
  vec3 pos_world = probe_origin + t * probe_raydir;

  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 albedo = subpassLoad(gchannel1).rgb;

  result = get_indirect_luminance(
    pos_world,
    N,
    frame.data,
    true, // prev
    probe_light_map_previous,
    probe_depth_map_previous,
    probe_attention,
    albedo
  );

  if (frame.data.flags.disable_indirect_bounces) {
    result = vec3(0.0);
  }
}
