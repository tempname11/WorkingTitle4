#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/lifetime.hxx>
#include "../mesh.hxx"
#include "../texture.hxx"
#include "../group.hxx"

namespace engine::loading::group {

struct DestroyData {
  lib::GUID group_id;
};

void _remove_scene_items(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Scene> scene,
  Use<SessionData::MetaMeshes> meta_meshes,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures,
  Ref<RenderingData::InflightGPU> inflight_gpu,
  Own<DestroyData> data
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->group_id == data->group_id) {
      engine::loading::mesh::deref(
        item->mesh_id,
        ctx,
        session,
        inflight_gpu,
        meta_meshes
      );

      engine::loading::texture::deref(
        item->texture_albedo_id,
        core,
        textures,
        meta_textures
      );

      engine::loading::texture::deref(
        item->texture_normal_id,
        core,
        textures,
        meta_textures
      );

      engine::loading::texture::deref(
        item->texture_romeao_id,
        core,
        textures,
        meta_textures
      );

      *item = scene->items.back();
      scene->items.pop_back();
      i--;
    }
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  free(data.ptr);
}

void _destroy(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Ref<RenderingData::InflightGPU> inflight_gpu,
  Own<DestroyData> data
) {
  ZoneScoped;

  std::unique_lock lock(session->groups.rw_mutex);
  session->groups.items.erase(data->group_id);

  auto task_remove_scene_items = lib::task::create(
    _remove_scene_items,
    session.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->meta_meshes,
    &session->vulkan.textures,
    &session->meta_textures,
    inflight_gpu.ptr,
    data.ptr
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    if (inflight_gpu.ptr != nullptr) {
      for (auto signal : inflight_gpu->signals) {
        dependencies.push_back({ signal, task_remove_scene_items });
      }
    }
    lib::task::inject(ctx->runner, {
      task_remove_scene_items
    }, {
      .new_dependencies = dependencies,
    });
  }
}

lib::GUID create(
  lib::task::ContextBase *ctx,
  Ref<SessionData> session,
  Ref<RenderingData::InflightGPU> inflight_gpu,
  GroupDescription *desc
) {
  ZoneScoped;

  lib::GUID group_id = lib::guid::next(&session->guid_counter);
  lib::lifetime::ref(&session->lifetime);
  lib::Task *yarn = nullptr;
  {
    std::unique_lock lock(session->groups.rw_mutex);
    auto result = session->groups.items.insert({ group_id, SessionData::Groups::Item {
      .name = desc->name,
    }});
    auto item = &(*result.first).second;
    lib::lifetime::init(&item->lifetime);
    yarn = item->lifetime.yarn;
  }

  auto data = new DestroyData {
    .group_id = group_id,
  };

  auto task_destroy = lib::task::create(
    _destroy,
    session.ptr,
    inflight_gpu.ptr,
    data
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_destroy,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { yarn, task_destroy },
  });

  return group_id;
}

} // namespace
