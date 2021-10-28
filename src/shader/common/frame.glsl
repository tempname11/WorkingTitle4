#ifndef _COMMON_FRAME_GLSL_
#define _COMMON_FRAME_GLSL_

struct FrameFlags {
  bool disable_direct_lighting;
  bool disable_indirect_lighting;
  bool disable_indirect_bounces;
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
  vec3 world_position_delta;
};

const uint PROBE_CASCADE_COUNT = 8; // :ProbeCascadeCount
const uvec2 PROBE_CASCADE_COUNT_FACTORS = uvec2(2, 4);

struct FrameProbe {
  mat3 random_orientation;
  uvec3 grid_size;
  uvec2 grid_size_z_factors;
  FrameProbeCascade cascades[PROBE_CASCADE_COUNT];
  vec3 grid_world_position_zero;
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