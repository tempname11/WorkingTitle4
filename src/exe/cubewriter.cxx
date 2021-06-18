#include <glm/glm.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <src/lib/gfx/mesh.hxx>

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

const glm::vec3 cube_tangents[12] = {
  // -XY
  {-1.0f, 0.0f, 0.0f},
  {-1.0f, 0.0f, 0.0f},
  // +XY
  {+1.0f, 0.0f, 0.0f},
  {+1.0f, 0.0f, 0.0f},
  // -YZ
  {0.0f, -1.0f, 0.0f},
  {0.0f, -1.0f, 0.0f},
  // +YZ
  {0.0f, +1.0f, 0.0f},
  {0.0f, +1.0f, 0.0f},
  // -ZX
  {0.0f, 0.0f, -1.0f},
  {0.0f, 0.0f, -1.0f},
  // +ZX
  {0.0f, 0.0f, +1.0f},
  {0.0f, 0.0f, +1.0f},
};

const glm::vec3 cube_normals[12] = {
  // -XY
  {0.0f, 0.0f, -1.0f},
  {0.0f, 0.0f, -1.0f},
  // +XY
  {0.0f, 0.0f, +1.0f},
  {0.0f, 0.0f, +1.0f},
  // -YZ
  {-1.0f, 0.0f, 0.0f},
  {-1.0f, 0.0f, 0.0f},
  // +YZ
  {+1.0f, 0.0f, 0.0f},
  {+1.0f, 0.0f, 0.0f},
  // -ZX
  {0.0f, -1.0f, 0.0f},
  {0.0f, -1.0f, 0.0f},
  // +ZX
  {0.0f, +1.0f, 0.0f},
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

int main(int argc, char** argv) {
  const uint32_t triangle_count = sizeof(cube_normals) / sizeof(*cube_normals);

  if (argc != 2) {
    fprintf(
      stderr,
      "USAGE: %s {out-filename}\n\n"
      "  Writes the cube triangles into {out-filename} (t04 format)\n",
      argv[0]
    );
    return EXIT_FAILURE;
  }

  FILE *out = fopen(argv[1], "wb");
  assert(out != nullptr);
  fwrite(&triangle_count, 1, sizeof(triangle_count), out);
  float zero = 0.0;
  for (size_t i = 0; i < 3 * triangle_count; i++) {
    mesh::VertexT05 v = {};
    v.position = cube_vertices[i];
    v.tangent = cube_tangents[i / 3];
    v.normal = cube_normals[i / 3];
    v.uv = cube_texcoords[i];
    fwrite(&v, 1, sizeof(v), out);
  }

  auto code = ferror(out);
  fclose(out);

  return code;
}