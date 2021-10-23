#pragma once
#include <glm/glm.hpp>

namespace engine::step::probe_measure {

struct PerCascade {
  glm::vec3 world_position_delta;
  uint32_t level;
};

} // namespace