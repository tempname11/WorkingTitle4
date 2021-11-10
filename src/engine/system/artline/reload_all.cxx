#include <src/lib/task.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

void reload_all(
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  assert(false); //!
  /*
  auto it = &session->artline;
  std::unique_lock lock(it->rw_mutex);
  for (auto &pair : it->dlls) {
    auto dll_id = pair.first;
    auto dll_item = &pair.second;
    assert(dll_item->status == Status::Ready);
    dll_item->status = Status::Reloading;

    auto load_data = new LoadData {
      .dll_filename = dll_item->filename,
      .dll_id = dll_id,
      .yarn_end = nullptr,
    };

    auto task_load = lib::task::create(
      _load,
      load_data,
      session.ptr
    );
    auto unload_data = new UnloadData {
      .dll_id = dll_id,
    };

    auto task_unload_scene = lib::task::create(
      _unload_scene,
      unload_data,
      &session->scene,
      session.ptr
    );
    auto task_unload_assets = lib::task::create(
      _unload_assets,
      unload_data,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      session.ptr
    );
    auto task_unload_assets_safe = lib::task::create(
      common::after_inflight,
      session.ptr,
      task_unload_assets
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_load,
      task_unload_scene,
      task_unload_assets_safe,
    });
    ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
      { task_load, task_unload_scene },
      { task_unload_scene, task_unload_assets_safe },
    });

    lib::lifetime::ref(&session->lifetime); // for unload
    lib::lifetime::ref(&session->lifetime); // for load
  }
  */
}

} // namespace
