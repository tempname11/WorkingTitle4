#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

void _remove_entry(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<UnloadData> data,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  assert(false); //!
  /*
  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);

    assert(it->dlls[data->dll_id].status == Status::Ready);
    it->dlls.erase(data->dll_id);
  }
  */
}

void unload(
  lib::GUID dll_id,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  ZoneScoped;

  auto data = new UnloadData {
    .dll_id = dll_id,
  };

  auto task_remove_entry = lib::task::create(
    _remove_entry,
    data,
    session.ptr
  );
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
    task_remove_entry, // @Think: iffy to do it async here?
    task_unload_scene,
    task_unload_assets,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_remove_entry, task_unload_scene },
    { task_unload_scene, task_unload_assets },
  });

  lib::lifetime::ref(&session->lifetime);
}

} // namespace
