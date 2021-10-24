#include <glm/glm.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <src/engine/common/mesh.hxx>

namespace engine::tools {

const glm::vec3 cube_vertices[36] = {
  // -XY
  {-1.0f, -1.0f, -1.0f},
  {+1.0f, -1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {-1.0f, +1.0f, -1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +XY
  {-1.0f, -1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {-1.0f, -1.0f, +1.0f},
  // -YZ
  {-1.0f, -1.0f, -1.0f},
  {-1.0f, +1.0f, -1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, -1.0f, +1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +YZ
  {+1.0f, -1.0f, -1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, -1.0f, -1.0f},
  // -ZX
  {-1.0f, -1.0f, -1.0f},
  {-1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, -1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +ZX
  {-1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, -1.0f},
};

const glm::vec3 cube_tangents[6] = {
  // -XY
  {-1.0f, 0.0f, 0.0f},
  // +XY
  {+1.0f, 0.0f, 0.0f},
  // -YZ
  {0.0f, -1.0f, 0.0f},
  // +YZ
  {0.0f, +1.0f, 0.0f},
  // -ZX
  {0.0f, 0.0f, -1.0f},
  // +ZX
  {0.0f, 0.0f, +1.0f},
};

const glm::vec3 cube_normals[6] = {
  // -XY
  {0.0f, 0.0f, -1.0f},
  // +XY
  {0.0f, 0.0f, +1.0f},
  // -YZ
  {-1.0f, 0.0f, 0.0f},
  // +YZ
  {+1.0f, 0.0f, 0.0f},
  // -ZX
  {0.0f, -1.0f, 0.0f},
  // +ZX
  {0.0f, +1.0f, 0.0f},
};

const glm::vec2 cube_texcoords[36] = {
  // -XY
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
  // +XY
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
  // -YZ
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
  // +YZ
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
  // -ZX
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
  // +ZX
  {-1.0f, -1.0f},
  {+1.0f, -1.0f},
  {+1.0f, +1.0f},
  {+1.0f, +1.0f},
  {-1.0f, +1.0f},
  {-1.0f, -1.0f},
};

void cube_writer(
  char const *path_t06
) {
  const uint32_t vertex_count = sizeof(cube_vertices) / sizeof(*cube_vertices);
  const uint32_t index_count = vertex_count;
  // all vertices are unique because the texcoords and normals differ at seams

  FILE *out = fopen(path_t06, "wb");
  assert(out != nullptr);

  fwrite(&index_count, 1, sizeof(index_count), out);
  fwrite(&vertex_count, 1, sizeof(vertex_count), out);

  for (uint16_t i = 0; i < index_count; i++) {
    fwrite(&i, 1, sizeof(i), out);
  }

  for (size_t i = 0; i < vertex_count; i++) {
    auto s = i / 6; // side
    engine::common::mesh::VertexT06 v = {};
    v.position = cube_vertices[i];
    v.tangent = cube_tangents[s];
    v.bitangent = glm::cross(cube_normals[s], cube_tangents[s]);
    v.normal = cube_normals[s];
    v.uv = cube_texcoords[i];
    fwrite(&v, 1, sizeof(v), out);
  }

  auto code = ferror(out);
  assert(code == 0);

  fclose(out);
}

} // namespace