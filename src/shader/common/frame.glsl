#ifndef _COMMON_FRAME_GLSL_
#define _COMMON_FRAME_GLSL_

struct FrameFlags {
  bool disable_direct_lighting;
  bool disable_indirect_lighting;
  bool disable_multiple_bounces;
  bool debug_A;
  bool debug_B;
  bool debug_C;
};

struct FrameProbe {
  mat3 random_orientation;
  uvec3 grid_size;
  uvec2 grid_size_z_factors;
  ivec3 change_from_prev;
  vec3 grid_world_position_zero;
  vec3 grid_world_position_zero_prev;
  vec3 grid_world_position_delta;
  vec2 light_map_texel_size;
  vec2 depth_map_texel_size;
  vec2 secondary_gbuffer_texel_size; // @Cleanup move this out of Probe?
  float depth_sharpness;
  float normal_bias;
};

struct FrameData {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse; 
  vec3 sky_sun_direction;
  vec3 sky_intensity;
  bool is_frame_sequential;
  FrameFlags flags;
  FrameProbe probe;
  uint end_marker;
};

#endif // _COMMON_FRAME_GLSL_