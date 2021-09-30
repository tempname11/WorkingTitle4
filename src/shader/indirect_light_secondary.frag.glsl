#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
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
layout(binding = 5) uniform Frame { FrameData data; } frame;

void main() {
  if (!frame.data.is_frame_sequential) {
    result = vec3(0.0);
    return;
  }

  // @Cleanup: share this code with other L2 passes, they have a lot in common.
  uvec2 texel_coord = uvec2(
    (position * 0.5 + 0.5) * frame.data.probe.secondary_gbuffer_texel_size
  );
  // Truncated, otherwise would need to subtract 0.5.

  // @CopyPaste :L2_ProbeCoord
  // @Performance: could do bit operations here if working with powers of 2.
  uvec2 ray_subcoord = texel_coord % probe_ray_count_factors;
  uint ray_index = ray_subcoord.x + ray_subcoord.y * probe_ray_count_factors.x;
  uvec2 combined_coord = texel_coord / probe_ray_count_factors;
  uvec3 probe_grid_coord = uvec3(
    combined_coord.xy % frame.data.probe.grid_size.xy,
    (combined_coord.x / frame.data.probe.grid_size.x) +
    (
      (combined_coord.y / frame.data.probe.grid_size.y)
        * frame.data.probe.grid_size_z_factors.x
    )
  );

  // everything is in world space.
  vec3 probe_origin = (
    frame.data.probe.grid_world_position_zero +
    frame.data.probe.grid_world_position_delta * probe_grid_coord
  );
  vec3 probe_raydir = get_probe_ray_direction(
    ray_index,
    probe_ray_count,
    frame.data.probe.random_orientation
  );
  float t = subpassLoad(zchannel).r;
  if (t == 0.0) { discard; }
  vec3 pos_world = probe_origin + t * probe_raydir;

  vec3 N = subpassLoad(gchannel0).rgb;

  result = get_indirect_luminance(
    pos_world,
    N,
    frame.data,
    frame.data.probe.grid_world_position_zero_prev,
    frame.data.probe.grid_world_position_delta,
    probe_light_map_previous
  );
}
