#include <src/lib/defer.hxx>
#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/easy_allocator.hxx>
#include <src/engine/common/after_inflight.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

void _update_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto scene_item = &scene->items[i];
    if (scene_item->owner_id == unload->dll_id) {
      scene->items[i] = scene->items[scene->items.size() - 1];
      scene->items.pop_back();
      i--;
    }
  }
}

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerUnload> unload,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  lib::lifetime::deref(&session->lifetime, ctx->runner);
  lib::easy_allocator::destroy(unload->misc);
}

void unload(
  lib::GUID dll_id,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {

  lib::array_t<lib::hash64_t> *mesh_keys = nullptr;
  lib::array_t<lib::hash64_t> *texture_keys = nullptr;
  {
    auto it = &session->artline;
    lib::mutex::lock(&it->mutex);

    auto key = lib::hash64::from_guid(dll_id);
    auto p_index = lib::u64_table::lookup(it->dll_indices, key);

    // Make sure Data::DLL is not longer needed.
    assert(p_index != nullptr);
    auto dll = &it->dlls->data[*p_index];
    assert(dll->status == Status::Ready);
    lib::cstr::crt_free(dll->file_path);

    // Move these arrays out, we'll use them once and destroy them afterwards.
    mesh_keys = dll->mesh_keys;
    texture_keys = dll->texture_keys;

    // Remove it from array.
    *dll = it->dlls->data[--it->dlls->count];

    // Update the index table.
    auto key_other = lib::hash64::from_guid(dll->id);
    auto p_index_other = lib::u64_table::lookup(it->dll_indices, key_other);
    assert(p_index_other != nullptr);
    *p_index_other = *p_index;

    // Finally, remove old index mapping.
    lib::u64_table::remove(it->dll_indices, key);

    lib::mutex::unlock(&it->mutex);
  }

  auto misc = lib::easy_allocator::create(1024 * 1024 * 1024); // 1 GB
  auto data = lib::allocator::make<PerUnload>(misc);
  *data = {
    .misc = misc,
    .dll_id = dll_id,
  };

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
    &session->vulkan.meshes,
    &session->vulkan.textures,
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
    task_update_scene.first,
    task_unload_assets.first,
    task_finish,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_update_scene.second, task_unload_assets.first },
    { task_unload_assets_third, task_finish },
  });

  lib::lifetime::ref(&session->lifetime);
}

} // namespace
