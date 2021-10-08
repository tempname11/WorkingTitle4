#pragma once
#include <glm/glm.hpp>

namespace engine::rendering::pass::probe_depth_update {

struct PerCascade {
  glm::vec3 world_position_delta;
  uint32_t level;
};

} // namespace