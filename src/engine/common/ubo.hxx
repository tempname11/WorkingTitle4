#pragma once
#include <glm/glm.hpp>

namespace engine::common::ubo {

struct Flags {
  uint32_t show_normals;
  uint32_t show_sky;
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

struct Material {
  alignas(16) glm::vec3 albedo;
  float metallic;
  float roughness;
  float ao;
};

} // namespace