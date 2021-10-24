#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/defer.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/common/texture.hxx>
#include "../texture.hxx"

namespace engine::loading::texture {

engine::common::texture::Data<uint8_t> load_rgba8(const char *filename) {
  int width, height, _channels = 4;
  auto data = stbi_load(filename, &width, &height, &_channels, 4);
  if (data == nullptr) {
    return {};
  };
  return engine::common::texture::Data<uint8_t> {
    .data = data,
    .width = width,
    .height = height,
    .channels = 4,
    .computed_mip_levels = lib::gfx::utilities::mip_levels(width, height),
  };
}

struct LoadData {
  lib::GUID texture_id;
  std::string path;
  VkFormat format;

  engine::common::texture::Data<uint8_t> the_texture;
  engine::session::Vulkan::Textures::Item texture_item;
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_texture = load_rgba8(data->path.c_str());
}

void _load_init_image(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::session::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Ref<lib::Task> signal,
  Own<LoadData> data 
) {
  ZoneScoped;

  VkImageCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = data->format,
    .extent = {
      .width = std::max(
        uint32_t(1), // for invalid image
        uint32_t(data->the_texture.width)
      ),
      .height = std::max(
        uint32_t(1), // for invalid image
        uint32_t(data->the_texture.height)
      ),
      .depth = 1,
    },
    .mipLevels = std::max(
      uint32_t(1), // for invalid image
      data->the_texture.computed_mip_levels
    ),
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  auto result = engine::uploader::prepare_image(
    &session->vulkan.uploader,
    core->device,
    core->allocator,
    &core->properties.basic,
    &create_info
  );
  data->texture_item.id = result.id;
  if (data->the_texture.data != nullptr) {
    memcpy(
      result.mem,
      data->the_texture.data,
      result.data_size
    );
  } else {
    memset(
      result.mem,
      0,
      result.data_size
    );
  }

  engine::uploader::upload_image(
    ctx,
    signal.ptr,
    &session->vulkan.uploader,
    session.ptr,
    &session->vulkan.core,
    &session->gpu_signal_support,
    queue_work,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    result.id
  );
}

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Vulkan::Textures> textures,
  Own<engine::session::Data::MetaTextures> meta_textures,
  Own<LoadData> data
) {
  ZoneScoped;

  textures->items.insert({ data->texture_id, data->texture_item });
  auto meta = &meta_textures->items.at(data->texture_id);
  meta->status = engine::session::Data::MetaTextures::Status::Ready;
  meta->will_have_loaded = nullptr;
  meta->invalid = data->the_texture.data == nullptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  stbi_image_free(data->the_texture.data);

  delete data.ptr;
}

lib::Task *load(
  std::string &path,
  VkFormat format,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaTextures> meta_textures,
  Use<engine::session::Data::GuidCounter> guid_counter,
  lib::GUID *out_texture_id
) {
  ZoneScoped;

  auto it = meta_textures->by_key.find({ path, format });
  if (it != meta_textures->by_key.end()) {
    auto texture_id = it->second;
    *out_texture_id = texture_id;

    auto meta = &meta_textures->items.at(texture_id);
    meta->ref_count++;

    if (meta->status == engine::session::Data::MetaTextures::Status::Loading) {
      assert(meta->will_have_loaded != nullptr);
      return meta->will_have_loaded;
    }

    if (meta->status == engine::session::Data::MetaTextures::Status::Ready) {
      return nullptr;
    }

    assert(false);
  }

  lib::lifetime::ref(&session->lifetime);

  auto texture_id = lib::guid::next(guid_counter.ptr);
  *out_texture_id = texture_id;
  meta_textures->by_key.insert({ { path, format }, texture_id });
  
  auto data = new LoadData {
    .texture_id = texture_id,
    .path = path,
    .format = format,
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
      &session->vulkan.core,
      &session->vulkan.queue_work,
      signal_init_image,
      data
    )
  );
  auto task_finish = lib::defer(
    lib::task::create(
      _load_finish,
      session.ptr,
      &session->vulkan.textures,
      &session->meta_textures,
      data
    )
  );
  
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_image.first,
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_image.first },
    { signal_init_image, task_finish.first },
  });

  engine::session::Data::MetaTextures::Item meta = {
    .ref_count = 1,
    .status = engine::session::Data::MetaTextures::Status::Loading,
    .will_have_loaded = task_finish.second,
    .path = path,
    .format = format,
  };
  meta_textures->items.insert({ texture_id, meta });

  return task_finish.second;
}

} // namespace
