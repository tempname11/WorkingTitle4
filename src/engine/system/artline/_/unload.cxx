#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "private.hxx"

namespace engine::system::artline {

void _unload_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto scene_item = &scene->items[i];
    if (scene_item->owner_id == data->dll_id) {
      data->items_removed.push_back(scene->items[i]);

      // remove item
      scene->items[i] = scene->items[scene->items.size() - 1];
      scene->items.pop_back();
      i--;
    }
  }
}

void _deref_texture(
  lib::GUID texture_id,
  artline::Data *it, // with lock
  Own<session::Vulkan::Meshes> meshes,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
) {
  assert(it->textures.contains(texture_id));
  auto texture_info = &it->textures.at(texture_id);
  assert(texture_info->ref_count > 0);
  texture_info->ref_count--;
  if (texture_info->ref_count == 0) {
    auto texture_item = &textures->items[texture_id];
    engine::uploader::destroy_image(
      &session->vulkan.uploader,
      &session->vulkan.core,
      texture_item->id
    );

    textures->items.erase(texture_id);

    it->textures_by_key.erase(texture_info->key);
    it->textures.erase(texture_id);
  }
}

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Own<session::Vulkan::Meshes> meshes,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);

    for (size_t i = 0; i < data->items_removed.size(); i++) {
      auto scene_item = &data->items_removed[i];

      {
        auto mesh_id = scene_item->mesh_id;
        assert(it->meshes.contains(mesh_id));
        auto mesh_info = &it->meshes.at(mesh_id);
        assert(mesh_info->ref_count > 0);
        mesh_info->ref_count--;
        if (mesh_info->ref_count == 0) {
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

          it->meshes_by_key.erase(mesh_info->key);
          it->meshes.erase(mesh_id);
        }
      }

      _deref_texture(
        scene_item->texture_albedo_id,
        it,
        meshes,
        textures,
        session
      );
      _deref_texture(
        scene_item->texture_normal_id,
        it,
        meshes,
        textures,
        session
      );
      _deref_texture(
        scene_item->texture_romeao_id,
        it,
        meshes,
        textures,
        session
      );
    }
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

} // namespace
