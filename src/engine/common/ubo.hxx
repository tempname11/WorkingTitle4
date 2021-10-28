#pragma once
#include <glm/glm.hpp>
#include <src/engine/constants.hxx>

namespace engine::common::ubo {

using gl_bool = uint32_t;

struct Flags {
  gl_bool disable_direct_lighting;
  gl_bool disable_indirect_lighting;
  gl_bool disable_indirect_bounces;
  gl_bool disable_eye_adaptation;
  gl_bool disable_motion_blur;
  gl_bool disable_TAA;
  gl_bool disable_sky;

  gl_bool debug_A;
  gl_bool debug_B;
  gl_bool debug_C;
};

struct ProbeCascade {
  glm::ivec3 infinite_grid_min;
  alignas(16) glm::ivec3 infinite_grid_min_prev;
  alignas(16) glm::vec3 grid_world_position_delta;
};

struct ProbeInfo {
  glm::mat3x4 random_orientation;
  alignas(16) glm::uvec3 grid_size;
  alignas(16) glm::uvec2 grid_size_z_factors;
  alignas(16) ProbeCascade cascades[PROBE_CASCADE_COUNT];
  alignas(16) glm::vec3 grid_world_position_zero;
  float depth_sharpness;
  float normal_bias;
};

struct Frame {
  glm::mat4 projection;
  glm::mat4 projection_prev;
  glm::mat4 view;
  glm::mat4 view_prev;
  glm::mat4 projection_inverse;
  glm::mat4 projection_prev_inverse;
  glm::mat4 view_inverse;
  glm::mat4 view_prev_inverse;
  float luminance_moving_average;
  alignas(16) glm::vec3 sky_sun_direction;
  alignas(16) glm::vec3 sky_intensity;
  uint32_t is_frame_sequential;
  Flags flags;
  alignas(16) ProbeInfo probe_info;
  alignas(16) uint32_t end_marker;
};

struct DirectionalLight {
  glm::vec3 direction;
  alignas(16) glm::vec3 irradiance;
};

} // namespace