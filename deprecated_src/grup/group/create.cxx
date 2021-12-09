#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/lifetime.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "../mesh.hxx"
#include "../texture.hxx"
#include "../group.hxx"
#include "../data.hxx"

namespace engine::system::grup::group {

struct DestroyData {
  lib::GUID group_id;
};

void _remove_scene_items(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::VulkanData::Core> core,
  Own<engine::session::Data::Scene> scene,
  Use<MetaMeshes> meta_meshes,
  Use<MetaTextures> meta_textures,
  Own<DestroyData> data
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->owner_id == data->group_id) {
      engine::system::grup::mesh::deref(
        item->mesh_id,
        ctx,
        session,
        meta_meshes
      );

      engine::system::grup::texture::deref(
        item->texture_albedo_id,
        ctx,
        session,
        meta_textures
      );

      engine::system::grup::texture::deref(
        item->texture_normal_id,
        ctx,
        session,
        meta_textures
      );

      engine::system::grup::texture::deref(
        item->texture_romeao_id,
        ctx,
        session,
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
  Ref<engine::session::Data> session,
  Own<DestroyData> data
) {
  ZoneScoped;

  std::unique_lock lock(session->grup.groups->rw_mutex);
  session->grup.groups->items.erase(data->group_id);

  auto task_remove_scene_items = lib::task::create(
    _remove_scene_items,
    session.ptr,
    &session->vulkan->core,
    &session->scene,
    session->grup.meta_meshes,
    session->grup.meta_textures,
    data.ptr
  );
  {
    std::scoped_lock lock(session->inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    for (auto signal : session->inflight_gpu->signals) {
      dependencies.push_back({ signal, task_remove_scene_items });
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
  Ref<engine::session::Data> session,
  GroupDescription *desc
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  lib::GUID group_id = lib::guid::next(session->guid_counter);
  lib::Task *yarn = nullptr;
  {
    std::unique_lock lock(session->grup.groups->rw_mutex);
    auto result = session->grup.groups->items.insert({
      group_id,
      engine::system::grup::Groups::Item {
        .name = desc->name,
      }
    });
    auto item = &(*result.first).second;
    lib::lifetime::init(&item->lifetime, 1);
    yarn = item->lifetime.yarn;
  }

  auto data = new DestroyData {
    .group_id = group_id,
  };

  auto task_destroy = lib::task::create(
    _destroy,
    session.ptr,
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