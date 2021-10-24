#pragma once
#include "../mesh.hxx"

namespace engine::loading::mesh {

struct LoadData {
  lib::GUID mesh_id;
  std::string path;
  engine::common::mesh::T06 the_mesh;
  engine::session::Vulkan::Meshes::Item mesh_item;
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
);

void _load_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Ref<lib::Task> signal,
  Own<LoadData> data 
);

void _load_init_blas(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Ref<lib::Task> signal,
  Own<LoadData> data 
);

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Vulkan::Meshes> meshes,
  Own<engine::session::Data::MetaMeshes> meta_meshes,
  Own<LoadData> data
);

} // namespace
