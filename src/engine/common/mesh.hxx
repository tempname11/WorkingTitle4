#pragma once
#include <glm/glm.hpp>

namespace engine::common::mesh {
  struct VertexT05 {
    glm::vec3 position; uint8_t _pad0[4];
    glm::vec3 tangent; uint8_t _pad1[4];
    glm::vec3 normal; uint8_t _pad2[4];
    glm::vec2 uv; uint8_t _pad3[8];
  };
  
  struct T05 {
    uint8_t *buffer;
    uint32_t triangle_count;
    VertexT05 *vertices;
  };
}