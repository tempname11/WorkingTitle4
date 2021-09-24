#ifndef _COMMON_FRAME_GLSL_
#define _COMMON_FRAME_GLSL_

struct FrameFlags {
  bool show_normals;
  bool show_sky;
  bool disable_direct_lighting;
  bool disable_indirect_lighting;
};

struct FrameProbe {
  uvec3 grid_size;
  uvec2 ray_pack_size;
  uint ray_count;
  vec3 grid_world_position_zero;
  vec3 grid_world_position_delta;
  vec2 light_map_texel_size;
  vec2 secondary_gbuffer_texel_size; // move this out of Probe?
};

struct FrameData {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
  FrameFlags flags;
  FrameProbe probe;
  uint end_marker;
};

#endif // _COMMON_FRAME_GLSL_