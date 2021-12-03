#pragma once
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/session/public.hxx>
#include <src/engine/session/data/vulkan.hxx>

namespace engine::system::artline {

struct PerLoadImpl;
struct PerLoad {
  lib::cstr_range_t dll_file_path;
  lib::cstr_range_t dll_file_path_copy;
  lib::GUID dll_id;
  lib::Task *yarn_done;
  lib::allocator_t *misc;

  PerLoadImpl *impl;

  struct Ready {
    lib::mutex_t mutex;
    lib::array_t<session::Data::Scene::Item> *scene_items;
    lib::array_t<lib::hash64_t> *mesh_keys;
    lib::array_t<lib::hash64_t> *texture_keys;
  } ready;
};

void _load_dll(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerLoad> data,
  Ref<session::Data> session
);

void _upload_mesh_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<common::mesh::T06> t06,
  Ref<session::VulkanData::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
);

void _upload_mesh_init_blas(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<session::VulkanData::Meshes::Item> mesh_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<session::Data> session
);

void _upload_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<VkFormat> format,
  Ref<common::texture::Data<uint8_t>> data,
  Ref<session::VulkanData::Textures::Item> texture_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<session::Data> session
);

struct PerUnload {
  lib::allocator_t *misc;
  lib::GUID dll_id;
};

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::array_t<lib::hash64_t>> mesh_keys,
  Ref<lib::array_t<lib::hash64_t>> texture_keys,
  Own<session::VulkanData::Meshes> meshes,
  Own<session::VulkanData::Textures> textures,
  Ref<session::Data> session
);

lib::array_t<common::mesh::T06> *generate_mc_v0(
  lib::allocator_t *misc,
  SignedDistanceFn *signed_distance_fn,
  TextureUvFn *texture_uv_fn
);

lib::array_t<common::mesh::T06> *generate_dc_v1(
  lib::allocator_t *misc,
  SignedDistanceFn *signed_distance_fn,
  TextureUvFn *texture_uv_fn,
  DualContouringParams *params
);

} // namespace
