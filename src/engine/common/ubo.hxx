#pragma once
#include <glm/glm.hpp>

namespace engine::common::ubo {

struct Flags {
  uint32_t show_normals;
  uint32_t show_sky;
  uint32_t disable_direct_lighting;
  uint32_t disable_indirect_lighting;
};

struct Frame {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 projection_inverse;
  glm::mat4 view_inverse;
  Flags flags;
  alignas(16) uint32_t end_marker;
};

struct DirectionalLight {
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 intensity;
};

} // namespace