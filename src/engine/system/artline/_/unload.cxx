#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "private.hxx"

namespace engine::system::artline {

void _deref_mesh(
  lib::hash64_t key,
  Data *it,
  Own<session::VulkanData::Meshes> meshes,
  Ref<session::Data> session
) {
  auto cached = lib::u64_table::lookup(it->meshes_by_key, key);
  assert(cached != nullptr);
  assert(cached->ref_count > 0);
  cached->ref_count--;
  if (cached->ref_count == 0) {
    for (size_t i = 0; i < cached->mesh_ids->count; i++) {
      auto mesh_id = cached->mesh_ids->data[i];
      auto mesh_item = &meshes->items[mesh_id];

      engine::uploader::destroy_buffer(
        session->vulkan->uploader,
        &session->vulkan->core,
        mesh_item->id
      );

      engine::blas_storage::destroy(
        session->vulkan->blas_storage,
        &session->vulkan->core,
        mesh_item->blas_id
      );

      meshes->items.erase(mesh_id);
    }

    lib::u64_table::remove(it->meshes_by_key, key);
  }
}

void _deref_texture(
  lib::hash64_t key,
  Data *it,
  Own<session::VulkanData::Textures> textures,
  Ref<session::Data> session
) {
  auto cached = lib::u64_table::lookup(it->textures_by_key, key);
  assert(cached != nullptr);
  assert(cached->ref_count > 0);
  cached->ref_count--;
  if (cached->ref_count == 0) {
    auto texture_item = &textures->items[cached->texture_id];

    engine::uploader::destroy_image(
      session->vulkan->uploader,
      &session->vulkan->core,
      texture_item->id
    );

    textures->items.erase(cached->texture_id);
    lib::u64_table::remove(it->textures_by_key, key);
  }
}

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::array_t<lib::hash64_t>> mesh_keys,
  Ref<lib::array_t<lib::hash64_t>> texture_keys,
  Own<session::VulkanData::Meshes> meshes,
  Own<session::VulkanData::Textures> textures,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto it = &session->artline;
    lib::mutex::lock(&it->mutex);

    for (size_t i = 0; i < mesh_keys->count; i++) {
      _deref_mesh(
        mesh_keys->data[i],
        it,
        meshes,
        session
      );
    }

    for (size_t i = 0; i < mesh_keys->count; i++) {
      _deref_texture(
        texture_keys->data[i],
        it,
        textures,
        session
      );
    }

    lib::mutex::unlock(&it->mutex);
  }

  lib::array::destroy(texture_keys.ptr);
  lib::array::destroy(mesh_keys.ptr);
}

} // namespace
