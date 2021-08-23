#include <src/engine/uploader.hxx>
#include "../mesh.hxx"

namespace engine::loading::mesh {

engine::common::mesh::T05 zero_t05 = {};

engine::common::mesh::T05 read_t05_file(const char *filename) {
  ZoneScoped;
  auto file = fopen(filename, "rb");

  // assert(file != nullptr);
  if (file == nullptr) {
    return zero_t05;
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = (uint8_t *) malloc(size);
  fread((void *) buffer, 1, size, file);

  // assert(ferror(file) == 0);
  if (ferror(file) != 0) {
    fclose(file);
    free(buffer);
    return zero_t05;
  }

  fclose(file);

  auto triangle_count = *((uint32_t*) buffer);

  // assert(size == sizeof(uint32_t) + sizeof(engine::common::mesh::VertexT05) * 3 * triangle_count);
  if (size != sizeof(uint32_t) + sizeof(engine::common::mesh::VertexT05) * 3 * triangle_count) {
    free(buffer);
    return zero_t05;
  }

  return {
    .buffer = buffer,
    .triangle_count = triangle_count,
    // three vertices per triangle
    .vertices = ((engine::common::mesh::VertexT05 *) (buffer + sizeof(uint32_t))),
  };
}

void deinit_t05(engine::common::mesh::T05 *it) {
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
  engine::uploader::destroy_buffer(
    &session->vulkan.uploader,
    core.ptr,
    item->id
  );
}

} // namespace
