#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "private.hxx"

namespace engine::system::artline {

void _unload_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto scene_item = &scene->items[i];
    if (scene_item->owner_id == unload->dll_id) {
      lib::array::ensure_space(&unload->items_removed, 1);
      unload->items_removed->data[unload->items_removed->count++] = scene->items[i];

      scene->items[i] = scene->items[scene->items.size() - 1];
      scene->items.pop_back();
      i--;
    }
  }
}

void _deref_mesh(
  lib::GUID mesh_id,
  Data *it,
  Own<session::Vulkan::Meshes> meshes,
  Ref<session::Data> session
) {
  auto key = lib::hash64::from_guid(mesh_id);
  auto cached = lib::u64_table::lookup(it->meshes_by_key, key);
  assert(cached != nullptr);
  assert(cached->ref_count > 0);
  cached->ref_count--;
  if (cached->ref_count == 0) {
    auto mesh_item = &meshes->items[mesh_id];

    engine::uploader::destroy_buffer(
      &session->vulkan.uploader,
      &session->vulkan.core,
      mesh_item->id
    );

    engine::blas_storage::destroy(
      &session->vulkan.blas_storage,
      &session->vulkan.core,
      mesh_item->blas_id
    );

    meshes->items.erase(mesh_id);
    lib::u64_table::remove(it->meshes_by_key, key);
  }
}

void _deref_texture(
  lib::GUID texture_id,
  Data *it,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
) {
  auto key = lib::hash64::from_guid(texture_id);
  auto cached = lib::u64_table::lookup(it->textures_by_key, key);
  assert(cached != nullptr);
  assert(cached->ref_count > 0);
  cached->ref_count--;
  if (cached->ref_count == 0) {
    auto texture_item = &textures->items[texture_id];

    engine::uploader::destroy_image(
      &session->vulkan.uploader,
      &session->vulkan.core,
      texture_item->id
    );

    textures->items.erase(texture_id);
    lib::u64_table::remove(it->textures_by_key, key);
  }
}

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Own<session::Vulkan::Meshes> meshes,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto it = &session->artline;
    lib::mutex::lock(&it->mutex);

    for (size_t i = 0; i < unload->items_removed->count; i++) {
      auto scene_item = &unload->items_removed->data[i];

      _deref_mesh(
        scene_item->mesh_id,
        it,
        meshes,
        session
      );
      _deref_texture(
        scene_item->texture_albedo_id,
        it,
        textures,
        session
      );
      _deref_texture(
        scene_item->texture_normal_id,
        it,
        textures,
        session
      );
      _deref_texture(
        scene_item->texture_romeao_id,
        it,
        textures,
        session
      );
    }

    lib::mutex::unlock(&it->mutex);
  }
}

} // namespace
