#pragma once
#include <glm/glm.hpp>

namespace engine::common::ubo {

struct Flags {
  uint32_t show_normals;
  uint32_t show_sky;
  uint32_t disable_direct_lighting;
  uint32_t disable_indirect_lighting;
};

struct ProbeInfo {
  alignas(16) glm::uvec3 grid_size;
  alignas(16) glm::uvec2 ray_pack_size;
  uint32_t ray_count;
  alignas(16) glm::vec3 grid_world_position_zero;
  alignas(16) glm::vec3 grid_world_position_delta;
  alignas(16) glm::vec2 light_map_texel_size;
  glm::vec2 secondary_gbuffer_texel_size;
};

struct Frame {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 projection_inverse;
  glm::mat4 view_inverse;
  Flags flags;
  ProbeInfo probe_info;
  alignas(16) uint32_t end_marker;
};

struct DirectionalLight {
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 intensity;
};

} // namespace