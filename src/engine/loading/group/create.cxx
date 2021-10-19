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
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::session::Data::Scene> scene,
  Use<engine::session::Data::MetaMeshes> meta_meshes,
  Use<engine::session::Data::MetaTextures> meta_textures,
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
        meta_meshes
      );

      engine::loading::texture::deref(
        item->texture_albedo_id,
        ctx,
        session,
        meta_textures
      );

      engine::loading::texture::deref(
        item->texture_normal_id,
        ctx,
        session,
        meta_textures
      );

      engine::loading::texture::deref(
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

  std::unique_lock lock(session->groups.rw_mutex);
  session->groups.items.erase(data->group_id);

  auto task_remove_scene_items = lib::task::create(
    _remove_scene_items,
    session.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->meta_meshes,
    &session->meta_textures,
    data.ptr
  );
  {
    std::scoped_lock lock(session->inflight_gpu.mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    for (auto signal : session->inflight_gpu.signals) {
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

  lib::GUID group_id = lib::guid::next(&session->guid_counter);
  lib::Task *yarn = nullptr;
  {
    std::unique_lock lock(session->groups.rw_mutex);
    auto result = session->groups.items.insert({ group_id, engine::session::Data::Groups::Item {
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
