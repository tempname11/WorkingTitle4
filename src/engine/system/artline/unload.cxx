#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

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
    // it->dlls[dll_id].status = Status::Unloading;
    it->dlls.erase(dll_id);
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
