#include <stb_image.h>
#include <src/lib/defer.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/common/after_inflight.hxx>
#include "../data.hxx"
#include "../texture.hxx"
#include "load.hxx"

namespace engine::system::grup::texture {

void _reload_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::VulkanData::Core> core,
  Own<engine::session::VulkanData::Textures> textures,
  Own<MetaTextures> meta_textures,
  Own<LoadData> data
) {
  ZoneScoped;

  auto item = &textures->items.at(data->texture_id);
  auto old_item = *item; // copy old data
  *item = data->texture_item; // replace the data

  engine::uploader::destroy_image(
    session->vulkan->uploader,
    core,
    old_item.id
  );

  auto meta = &meta_textures->items.at(data->texture_id);
  meta->invalid = data->the_texture.data == nullptr;
  meta->will_have_reloaded = nullptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  stbi_image_free(data->the_texture.data);

  delete data.ptr;
}

void reload(
  lib::GUID texture_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<MetaTextures> meta_textures
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_textures->items.at(texture_id);
  assert(meta->ref_count > 0);

  // Reloading will likely only ever be manual, and right now the user
  // would simply have to wait to reload something that's already reloading.
  // We have to ensure the status is right before calling this.
  assert(meta->status == MetaTextures::Status::Ready);

  auto data = new LoadData {
    .texture_id = texture_id,
    .path = meta->path,
    .format = meta->format,
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto signal_init_image = lib::task::create_external_signal();
  auto task_init_image = lib::defer(
    lib::task::create(
      _load_init_image,
      session.ptr,
      &session->vulkan->core,
      &session->vulkan->queue_work,
      signal_init_image,
      data
    )
  );
  auto task_finish = lib::task::create(
    common::after_inflight,
    session.ptr,
    lib::task::create(
      _reload_finish,
      session.ptr,
      &session->vulkan->core,
      &session->vulkan->textures,
      session->grup.meta_textures,
      data
    )
  );

  assert(!meta->will_have_reloaded);
  meta->will_have_reloaded = task_finish;

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_image.first,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_image.first },
    { signal_init_image, task_finish },
  });
}

} // namespace
