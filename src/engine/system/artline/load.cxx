#include <src/lib/task.hxx>
#include <src/lib/defer.hxx>
#include <src/lib/easy_allocator.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

void _update_scene(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  for (size_t i = 0; i < data->ready->scene_items->count; i++) {
    scene->items.push_back(data->ready->scene_items->data[i]);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  lib::easy_allocator::destroy(data->misc);
}

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<session::Data> session
) {
  ZoneScoped;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  lib::easy_allocator::destroy(data->misc);
}

lib::Task *load(
  lib::cstr_range_t dll_filename,
  Ref<session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto misc = lib::easy_allocator::create(1024 * 1024 * 1024); // 1 GB
  auto yarn_done = lib::task::create_yarn_signal();
  auto dll_id = lib::guid::next(&session->guid_counter);
  auto data = lib::allocator::make<LoadData>(misc);
  *data = {
    .dll_filename = dll_filename,
    .dll_id = dll_id,
    .yarn_done = yarn_done,
    .misc = misc,
  };

  auto ready_data = lib::allocator::make<ReadyData>(misc);
  *ready_data = {
    .scene_items = lib::array::create<session::Data::Scene::Item>(misc, 0),
  };
  lib::mutex::init(&ready_data->mutex);

  auto task_load_dll = lib::task::create(
    _load_dll,
    data,
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

  auto task_finish = lib::defer(
    lib::task::create(
      _finish,
      data,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_load_dll,
    task_update_scene.first,
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { yarn_done, task_update_scene.first },
    { task_update_scene.second, task_finish.first },
  });

  {
    auto it = &session->artline;
    lib::mutex::lock(&it->mutex);

    lib::u64_table::insert(
      &it->dlls,
      lib::u64_table::from_guid(dll_id),
      Data::DLL {
        .filename = lib::cstr::crt_copy(dll_filename),
        .status = Status::Loading,
      }
    );
      
    lib::mutex::unlock(&it->mutex);
  }

  lib::lifetime::ref(&session->lifetime);
  return task_finish.second;
}

} // namespace
