#include <src/lib/defer.hxx>
#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/easy_allocator.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  lib::lifetime::deref(&session->lifetime, ctx->runner);
  lib::easy_allocator::destroy(unload->misc);
}

void _remove_entry(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  auto it = &session->artline;
  lib::mutex::lock(&it->mutex);
  auto key = lib::hash64::from_guid(unload->dll_id);
  auto dll = lib::u64_table::lookup(it->dlls, key);
  assert(dll != nullptr);
  assert(dll->status == Status::Ready);
  lib::cstr::crt_free(dll->file_path);
  lib::u64_table::remove(it->dlls, key);
  lib::mutex::unlock(&it->mutex);
}

void unload(
  lib::GUID dll_id,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  ZoneScoped;

  auto misc = lib::easy_allocator::create(1024 * 1024 * 1024); // 1 GB
  auto data = lib::allocator::make<PerUnload>(misc);
  *data = {
    .misc = misc,
    .dll_id = dll_id,
    .items_removed = lib::array::create<session::Data::Scene::Item>(misc, 0),
  };

  auto task_remove_entry = lib::task::create(
    _remove_entry,
    data,
    session.ptr
  );
  auto task_unload_scene = lib::defer(
    lib::task::create(
      _unload_scene,
      data,
      &session->scene,
      session.ptr
    )
  );
  auto task_unload_assets = lib::defer(
    lib::task::create(
      common::after_inflight,
      session.ptr,
        lib::task::create(
        _unload_assets,
        data,
        &session->vulkan.meshes,
        &session->vulkan.textures,
        session.ptr
      )
    )
  );
  auto task_finish = lib::task::create(
    _finish,
    data,
    session.ptr
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_remove_entry,
    task_unload_scene.first,
    task_unload_assets.first,
    task_finish,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_remove_entry, task_unload_scene.first },
    { task_unload_scene.second, task_unload_assets.first },
    { task_unload_assets.second, task_finish },
  });

  lib::lifetime::ref(&session->lifetime);
}

} // namespace
