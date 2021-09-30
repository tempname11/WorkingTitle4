#pragma once
#include <glm/glm.hpp>

namespace engine::common::ubo {

using gl_bool = uint32_t;

struct Flags {
  gl_bool disable_direct_lighting;
  gl_bool disable_indirect_lighting;
  gl_bool debug_A;
  gl_bool debug_B;
  gl_bool debug_C;
};

struct ProbeInfo {
  glm::mat3x4 random_orientation;
  alignas(16) glm::uvec3 grid_size;
  alignas(16) glm::uvec2 grid_size_z_factors;
  alignas(16) glm::ivec3 change_from_prev;
  alignas(16) glm::vec3 grid_world_position_zero;
  alignas(16) glm::vec3 grid_world_position_zero_prev;
  alignas(16) glm::vec3 grid_world_position_delta;
  alignas(16) glm::vec2 light_map_texel_size;
  glm::vec2 secondary_gbuffer_texel_size;
};

struct Frame {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 projection_inverse;
  glm::mat4 view_inverse;
  uint32_t is_frame_sequential;
  alignas(16) Flags flags;
  alignas(16) ProbeInfo probe_info;
  alignas(16) uint32_t end_marker;
};

struct DirectionalLight {
  glm::vec3 direction;
  alignas(16) glm::vec3 intensity;
};

} // namespace