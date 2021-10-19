#include <src/task/after_inflight.hxx>
#include <src/task/defer.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "../mesh.hxx"
#include "common.hxx"
#include "load.hxx"

namespace engine::loading::mesh {

void _reload_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::session::Vulkan::Meshes> meshes,
  Own<engine::session::Data::MetaMeshes> meta_meshes,
  Own<LoadData> data
) {
  ZoneScoped;

  auto item = &meshes->items.at(data->mesh_id);
  auto old_item = *item; // copy old data
  *item = data->mesh_item; // replace the data

  engine::uploader::destroy_buffer(
    &session->vulkan.uploader,
    core,
    old_item.id
  );

  engine::blas_storage::destroy(
    &session->vulkan.blas_storage,
    core,
    old_item.blas_id
  );

  auto meta = &meta_meshes->items.at(data->mesh_id);
  meta->invalid = item->index_count == 0;
  meta->will_have_reloaded = nullptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  deinit_t06(&data->the_mesh);

  delete data.ptr;
}

void reload(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaMeshes> meta_meshes
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_meshes->items.at(mesh_id);
  assert(meta->ref_count > 0);

  // Reloading will likely only ever be manual, and right now the user
  // would simply have to wait to reload something that's already reloading.
  // We have to ensure the status is right before calling this.
  assert(meta->status == engine::session::Data::MetaMeshes::Status::Ready);

  auto data = new LoadData {
    .mesh_id = mesh_id,
    .path = meta->path,
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto signal_init_buffer = lib::task::create_external_signal();
  auto task_init_buffer = defer(
    lib::task::create(
      _load_init_buffer,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.queue_work,
      signal_init_buffer,
      data
    )
  );
  auto signal_init_blas = lib::task::create_external_signal();
  auto task_init_blas = defer(
    lib::task::create(
      _load_init_blas,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.queue_work,
      signal_init_blas,
      data
    )
  );
  auto task_finish = lib::task::create(
    after_inflight,
    session.ptr,
    lib::task::create(
      _reload_finish,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.meshes,
      &session->meta_meshes,
      data
    )
  );

  assert(!meta->will_have_reloaded);
  meta->will_have_reloaded = task_finish;

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer.first,
    task_init_blas.first,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer.first },
    { signal_init_buffer, task_init_blas.first },
    { signal_init_blas, task_finish },
  });
}

} // namespace
