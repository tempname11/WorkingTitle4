#pragma once
#include <glm/glm.hpp>
#include <src/lib/gfx/allocator.hxx>

namespace engine::common::mesh {
  using IndexT06 = uint16_t; // @See :T06IndexType

  struct VertexT06 { // @See :T06VertexData
    glm::vec3 position; uint8_t _pad0[4];
    glm::vec3 tangent; uint8_t _pad1[4];
    glm::vec3 bitangent; uint8_t _pad2[4];
    glm::vec3 normal; uint8_t _pad3[4];
    glm::vec2 uv; uint8_t _pad4[8];
  };
  
  struct T06 {
    uint8_t *buffer;
    uint32_t index_count;
    uint32_t vertex_count;
    IndexT06 *indices;
    VertexT06 *vertices;
  };
}