#pragma once
#include <string>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/session/public.hxx>

namespace engine::system::artline {

struct LoadData {
  std::string dll_filename;
  lib::GUID dll_id;
  lib::Task *yarn_end;
};

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<engine::session::Data> session
);

void _upload_mesh_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::common::mesh::T06> t06,
  Ref<engine::session::Vulkan::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
);

void _upload_mesh_init_blas(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
);

void _upload_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<VkFormat> format,
  Ref<engine::common::texture::Data<uint8_t>> data,
  Ref<engine::session::Vulkan::Textures::Item> texture_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
);

struct UnloadData {
  lib::GUID dll_id;
  std::vector<session::Data::Scene::Item> items_removed;
};

void _unload_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
);

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Own<session::Vulkan::Meshes> meshes,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
);

} // namespace
