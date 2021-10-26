#ifndef _COMMON_FRAME_GLSL_
#define _COMMON_FRAME_GLSL_

struct FrameFlags {
  bool disable_direct_lighting;
  bool disable_indirect_lighting;
  bool disable_indirect_bounces;
  bool disable_indirect_attention;
  bool disable_eye_adaptation;
  bool disable_motion_blur;
  bool disable_TAA;
  bool disable_sky;
  bool debug_A;
  bool debug_B;
  bool debug_C;
};

struct FrameProbeCascade {
  ivec3 infinite_grid_min;
  ivec3 infinite_grid_min_prev;
};

const uint MAX_CASCADE_LEVELS = 8; // :MaxCascadeLevels

struct FrameProbe {
  mat3 random_orientation;
  uvec3 grid_size;
  uint cascade_count;
  uvec2 grid_size_z_factors;
  uvec2 cascade_count_factors;
  FrameProbeCascade cascades[MAX_CASCADE_LEVELS];
  vec3 grid_world_position_zero;
  vec3 grid_world_position_delta_c0;
  float depth_sharpness;
  float normal_bias;
};

struct FrameData {
  mat4 projection;
  mat4 projection_prev;
  mat4 view;
  mat4 view_prev;
  mat4 projection_inverse;
  mat4 projection_prev_inverse;
  mat4 view_inverse; 
  mat4 view_prev_inverse; 
  float luminance_moving_average;
  vec3 sky_sun_direction;
  vec3 sky_intensity;
  bool is_frame_sequential;
  FrameFlags flags;
  FrameProbe probe;
  uint end_marker;
};

#endif // _COMMON_FRAME_GLSL_