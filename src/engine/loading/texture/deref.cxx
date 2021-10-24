#include <src/engine/uploader.hxx>
#include <src/engine/common/after_inflight.hxx>
#include "../mesh.hxx"

namespace engine::loading::texture {

struct DerefData {
  lib::GUID texture_id;
};

void _deref(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::Vulkan::Core> core,
  Own<engine::session::Vulkan::Textures> textures,
  Own<engine::session::Data::MetaTextures> meta_textures,
  Own<DerefData> data
) {
  ZoneScoped;

  auto meta = &meta_textures->items.at(data->texture_id);
  assert(meta->ref_count > 0);

  // we must have waited for the initial load to finish.
  assert(meta->status == engine::session::Data::MetaTextures::Status::Ready);

  // we must have waited for reload to finish.
  assert(meta->will_have_reloaded == nullptr);

  meta->ref_count--;
  if (meta->ref_count == 0) {
    auto texture = &textures->items.at(data->texture_id);
    engine::uploader::destroy_image(
      &session->vulkan.uploader,
      core.ptr,
      texture->id
    );

    meta_textures->by_key.erase({ meta->path, meta->format });
    meta_textures->items.erase(data->texture_id);
    textures->items.erase(data->texture_id);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

void deref(
  lib::GUID texture_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::MetaTextures> meta_textures
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_textures->items.at(texture_id);
  auto data = new DerefData {
    .texture_id = texture_id,
  };
  auto task_deref = lib::task::create(
    common::after_inflight,
    session.ptr,
    lib::task::create(
      _deref,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.textures,
      &session->meta_textures,
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
