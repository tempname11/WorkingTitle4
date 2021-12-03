#include <src/lib/task.hxx>
#include <src/lib/defer.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include <src/lib/easy_allocator.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

struct PerReload {
  PerLoad load;
  PerUnload unload;
};

void _update_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerReload> reload,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto scene_item = &scene->items[i];
    if (scene_item->owner_id == reload->unload.dll_id) {
      scene->items[i] = scene->items[scene->items.size() - 1];
      scene->items.pop_back();
      i--;
    }
  }

  for (size_t i = 0; i < reload->load.ready.scene_items->count; i++) {
    scene->items.push_back(reload->load.ready.scene_items->data[i]);
  }
}

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerReload> reload,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  {
    auto it = &session->artline;
    lib::mutex::lock(&it->mutex);

    auto p_index = lib::u64_table::lookup(
      it->dll_indices,
      lib::hash64::from_guid(reload->load.dll_id)
    );
    assert(p_index != nullptr);
    auto dll = &it->dlls->data[*p_index];
    dll->status = Status::Ready;
    dll->mesh_keys = reload->load.ready.mesh_keys;
    dll->texture_keys = reload->load.ready.texture_keys;
      
    lib::mutex::unlock(&it->mutex);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  assert(reload->unload.misc == reload->load.misc);
  lib::easy_allocator::destroy(reload->load.misc);
}

void _reload(
  Data::DLL *dll,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto mesh_keys = dll->mesh_keys;
  auto texture_keys = dll->texture_keys;
  dll->status = Status::Reloading;
  dll->mesh_keys = nullptr;
  dll->texture_keys = nullptr;

  auto misc = lib::easy_allocator::create(lib::allocator::GB);
  auto data = lib::allocator::make<PerReload>(misc);
  *data = {
    .load = {
      .dll_file_path = dll->file_path,
      .dll_id = dll->id,
      .yarn_done = lib::task::create_yarn_signal(),
      .misc = misc,
      .ready = {
        .scene_items = lib::array::create<session::Data::Scene::Item>(misc, 0),
        .mesh_keys = lib::array::create<lib::hash64_t>(lib::allocator::crt, 0),
        .texture_keys = lib::array::create<lib::hash64_t>(lib::allocator::crt, 0),
      },
    },
    .unload = {
      .misc = misc,
      .dll_id = dll->id,
    },
  };

  lib::mutex::init(&data->load.ready.mutex);
  auto task_load_dll = lib::task::create(
    _load_dll,
    &data->load,
    session.ptr
  );

  auto task_update_scene = lib::defer(
    lib::task::create(
      _update_scene,
      data,
      &session->scene,
      session.ptr
    )
  );

  auto task_unload_assets_third = lib::task::create(
    _unload_assets,
    mesh_keys,
    texture_keys,
    &session->vulkan->meshes,
    &session->vulkan->textures,
    session.ptr
  );
  auto task_unload_assets = lib::defer(
    lib::task::create(
      common::after_inflight,
      session.ptr,
      task_unload_assets_third
    )
  );

  auto task_finish = lib::task::create(
    _finish,
    data,
    session.ptr
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_load_dll,
    task_update_scene.first,
    task_unload_assets.first,
    task_finish,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { data->load.yarn_done, task_update_scene.first },
    { task_update_scene.second, task_unload_assets.first },
    { task_unload_assets_third, task_finish },
  });

  lib::lifetime::ref(&session->lifetime);
}

void reload_all(
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto it = &session->artline;
  lib::mutex::lock(&it->mutex);

  for (size_t i = 0; i < it->dlls->count; i++) {
    auto dll = &it->dlls->data[i];
    assert(dll->status == Status::Ready);
    _reload(dll, session, ctx);
  }

  lib::mutex::unlock(&it->mutex);
}

} // namespace
