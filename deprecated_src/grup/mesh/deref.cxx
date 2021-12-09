#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include <src/engine/common/after_inflight.hxx>
#include "../mesh.hxx"
#include "../data.hxx"
#include "common.hxx"

namespace engine::system::grup::mesh {

struct DerefData {
  lib::GUID mesh_id;
};

void _deref(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::VulkanData::Core> core,
  Own<engine::session::VulkanData::Meshes> meshes,
  Own<MetaMeshes> meta_meshes,
  Own<DerefData> data
) {
  ZoneScoped;

  auto meta = &meta_meshes->items.at(data->mesh_id);
  assert(meta->ref_count > 0);

  // we must have waited for the initial load to finish.
  assert(meta->status == MetaMeshes::Status::Ready);

  // we must have waited for reload to finish.
  assert(meta->will_have_reloaded == nullptr);

  meta->ref_count--;
  if (meta->ref_count == 0) {
    auto mesh = &meshes->items.at(data->mesh_id);

    engine::uploader::destroy_buffer(
      session->vulkan->uploader,
      core,
      mesh->id
    );

    engine::blas_storage::destroy(
      session->vulkan->blas_storage,
      core,
      mesh->blas_id
    );

    meta_meshes->by_path.erase(meta->path);
    meta_meshes->items.erase(data->mesh_id);
    meshes->items.erase(data->mesh_id);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

void deref(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Use<MetaMeshes> meta_meshes
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_meshes->items.at(mesh_id);
  auto data = new DerefData {
    .mesh_id = mesh_id,
  };
  auto task_deref = lib::task::create(
    common::after_inflight,
    session.ptr,
    lib::task::create(
      _deref,
      session.ptr,
      &session->vulkan->core,
      &session->vulkan->meshes,
      session->grup.meta_meshes,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_deref,
  });
  if (meta->will_have_loaded != nullptr) {
    ctx->new_dependencies.insert(ctx->new_dependencies.end(),
      { meta->will_have_loaded, task_deref }
    );
  }
  if (meta->will_have_reloaded != nullptr) {
    ctx->new_dependencies.insert(ctx->new_dependencies.end(),
      { meta->will_have_reloaded, task_deref }
    );
  }
}

} // namespace
