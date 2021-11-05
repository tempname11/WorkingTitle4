#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

struct UnloadData {
  lib::GUID dll_id;
  std::vector<session::Data::Scene::Item> items_removed;
};

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

void _unload_assets(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Own<session::Vulkan::Meshes> meshes,
  Own<session::Vulkan::Textures> textures,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < data->items_removed.size(); i++) {
    auto scene_item = &data->items_removed[i];

    {
      auto mesh_item = &meshes->items[scene_item->mesh_id];
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

      meshes->items.erase(scene_item->mesh_id);
    }

    { // albedo
      auto texture_item = &textures->items[scene_item->texture_albedo_id];
      engine::uploader::destroy_image(
        &session->vulkan.uploader,
        &session->vulkan.core,
        texture_item->id
      );

      textures->items.erase(scene_item->texture_albedo_id);
    }
      
    { // normal
      auto texture_item = &textures->items[scene_item->texture_normal_id];
      engine::uploader::destroy_image(
        &session->vulkan.uploader,
        &session->vulkan.core,
        texture_item->id
      );

      textures->items.erase(scene_item->texture_normal_id);
    }
      
    { // romeao
      auto texture_item = &textures->items[scene_item->texture_romeao_id];
      engine::uploader::destroy_image(
        &session->vulkan.uploader,
        &session->vulkan.core,
        texture_item->id
      );

      textures->items.erase(scene_item->texture_romeao_id);
    }
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

void unload(
  lib::GUID dll_id,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  ZoneScoped;

  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);

    assert(it->dlls[dll_id].status == Status::Ready);
    it->dlls[dll_id].status = Status::Unloading;
  }
  
  auto data = new UnloadData {
    .dll_id = dll_id,
  };

  auto task_unload_scene = lib::task::create(
    _unload_scene,
    data,
    &session->scene,
    session.ptr
  );
  auto task_unload_assets = lib::task::create(
    common::after_inflight,
    session.ptr,
      lib::task::create(
      _unload_assets,
      data,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_unload_scene,
    task_unload_assets,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_unload_scene, task_unload_assets },
  });

  lib::lifetime::ref(&session->lifetime);
}

} // namespace
