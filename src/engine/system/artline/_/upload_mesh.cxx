#include <vulkan/vulkan.h>
#include <src/lib/task.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>

namespace engine::system::artline {

void _upload_mesh_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::common::mesh::T06> t06,
  Ref<engine::session::Vulkan::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  auto core = &session->vulkan.core;

  auto aligned_size = lib::gfx::utilities::aligned_size(
    t06->index_count * sizeof(engine::common::mesh::IndexT06),
    sizeof(engine::common::mesh::VertexT06)
  );
  mesh_item->index_count = t06->index_count;
  mesh_item->buffer_offset_indices = 0;
  mesh_item->buffer_offset_vertices = aligned_size;
  VkBufferCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = std::max(
      size_t(1), // for invalid buffer
      (0
        + aligned_size
        + t06->vertex_count * sizeof(engine::common::mesh::VertexT06)
      )
    ),
    .usage = (0
      | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
      | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
      // usage below is for raytracing:
      | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
      | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
      | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    ),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  auto result = engine::uploader::prepare_buffer(
    &session->vulkan.uploader,
    core->device,
    core->allocator,
    &core->properties.basic,
    &create_info
  );

  mesh_item->id = result.id;
  if(t06->index_count > 0) {
    memcpy(
      result.mem,
      t06->indices,
      result.data_size
    );
  }

  engine::uploader::upload_buffer(
    ctx,
    signal.ptr,
    &session->vulkan.uploader,
    session.ptr,
    queue_work,
    result.id
  );
}

void _upload_mesh_init_blas(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  auto pair = engine::uploader::get_buffer(
    &session->vulkan.uploader,
    mesh_item->id
  );
  auto buffer = pair.first;

  engine::blas_storage::VertexInfo vertex_info = {
    .stride = sizeof(engine::common::mesh::VertexT06),
    .index_count = mesh_item->index_count,
    .buffer_offset_indices = mesh_item->buffer_offset_indices,
    .buffer_offset_vertices = mesh_item->buffer_offset_vertices,
    .buffer = buffer,
  };

  mesh_item->blas_id = engine::blas_storage::create(
    ctx,
    &session->vulkan.blas_storage,
    signal.ptr,
    &vertex_info,
    session,
    &session->vulkan.core
  );
}

} // namespace
