#include <src/lib/gfx/utilities.hxx>
#include "mesh.hxx"

namespace engine::tools::common::mesh {

void write(const char *out_filename, T06_Builder *mesh) {
  FILE *out = fopen(out_filename, "wb");
  assert(out != nullptr);
  uint32_t index_count = mesh->indices.size();
  uint32_t vertex_count = mesh->vertices.size();
  fwrite(&index_count, 1, sizeof(index_count), out);
  fwrite(&vertex_count, 1, sizeof(vertex_count), out);
  auto indices_size = index_count * sizeof(engine::common::mesh::IndexT06);
  auto aligned_size = lib::gfx::utilities::aligned_size(
    indices_size,
    sizeof(engine::common::mesh::VertexT06)
  );
  auto padding_size = aligned_size - indices_size;
  fwrite(mesh->indices.data(), 1, mesh->indices.size() * sizeof(engine::common::mesh::IndexT06), out);
  for (size_t i = 0; i < padding_size; i++) {
    uint8_t zero = 0;
    fwrite(&zero, 1, sizeof(zero), out);
  }
  fwrite(mesh->vertices.data(), 1, mesh->vertices.size() * sizeof(engine::common::mesh::VertexT06), out);
  assert(ferror(out) == 0);
  fclose(out);
}

} // namespace
