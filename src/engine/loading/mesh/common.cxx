#include <src/engine/uploader.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "../mesh.hxx"

namespace engine::loading::mesh {

engine::common::mesh::T06 read_t06_file(const char *filename) {
  ZoneScoped;
  auto file = fopen(filename, "rb");

  if (file == nullptr) {
    return {};
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = (uint8_t *) malloc(size);
  fread((void *) buffer, 1, size, file);

  if (ferror(file) != 0) {
    fclose(file);
    free(buffer);
    return {};
  }

  fclose(file);

  auto index_count = *((uint32_t*) buffer);
  auto vertex_count = *((uint32_t*) (buffer + sizeof(uint32_t)));
  auto indices_size = index_count * sizeof(engine::common::mesh::IndexT06);
  auto aligned_size = lib::gfx::utilities::aligned_size(
    indices_size,
    sizeof(engine::common::mesh::VertexT06)
  );

  if (
    size != (0
      + 2 * sizeof(uint32_t)
      + aligned_size
      + vertex_count * sizeof(engine::common::mesh::VertexT06)
    )
  ) {
    free(buffer);
    return {};
  }

  return {
    .buffer = buffer,
    .index_count = index_count,
    .vertex_count = vertex_count,
    .indices = (engine::common::mesh::IndexT06 *) (buffer
      + 2 * sizeof(uint32_t)
    ),
    .vertices = (engine::common::mesh::VertexT06 *) (buffer
      + 2 * sizeof(uint32_t)
      + aligned_size
    ),
  };
}

void deinit_t06(engine::common::mesh::T06 *it) {
  ZoneScoped;
  if (it->buffer != nullptr) {
    free(it->buffer);
  }
}

void _unload_item(
  SessionData::Vulkan::Meshes::Item *item,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core
) {
  // @Minor: this does not need to be a procedure
  engine::uploader::destroy_buffer(
    &session->vulkan.uploader,
    core,
    item->id
  );
}

} // namespace
