#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include "../mesh.hxx"
#include "../texture.hxx"
#include "../group.hxx"

namespace engine::loading::group {

struct UnloadData {
  lib::GUID group_id;
};

void _unload(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<UnloadData> unload_data,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Scene> scene,
  Use<SessionData::MetaMeshes> meta_meshes,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->group_id == unload_data->group_id) {
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

  free(unload_data.ptr);
}

void remove(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  lib::lifetime::ref(&session->lifetime);

  groups->items.erase(group_id);

  auto data = new UnloadData {
    .group_id = group_id,
  };
  auto task_unload = lib::task::create(
    _unload,
    data,
    session.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->meta_meshes,
    &session->vulkan.textures,
    &session->meta_textures,
    inflight_gpu.ptr
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    if (inflight_gpu.ptr != nullptr) {
      for (auto signal : inflight_gpu->signals) {
        dependencies.push_back({ signal, task_unload });
      }
    }
    lib::task::inject(ctx->runner, {
      task_unload
    }, {
      .new_dependencies = dependencies,
    });
  }
}

} // namespace
