#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <src/engine/texture.hxx>
#include <src/task/defer.hxx>
#include "texture.hxx"

namespace engine::loading::texture {

struct LoadData {
  lib::GUID texture_id;
  const char *path;
  size_t texel_size;
  VkFormat texture_format;
  engine::common::texture::Data<uint8_t> the_texture;
  SessionData::Vulkan::Textures::Item texture_item;
  lib::gfx::multi_alloc::Instance staging_multi_alloc;
  lib::gfx::multi_alloc::StakeBuffer staging_buffer;
  VkSemaphore semaphore_finished;
  VkCommandPool command_pool; // this needs to go later!
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_texture = engine::texture::load_rgba8(data->path);
}

void _load_init_image(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::Vulkan::Core> core,
  Use<lib::gpu_signal::Support> gpu_signal_support,
  Own<VkQueue> queue_work,
  Own<lib::Task> signal,
  Own<LoadData> data 
) {
  ZoneScoped;

  { ZoneScopedN("semaphore_finished");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    {
      auto result = vkCreateSemaphore(
        core->device,
        &create_info,
        core->allocator,
        &data->semaphore_finished
      );
      assert(result == VK_SUCCESS);
    }
  }

  { ZoneScopedN("command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = core->queue_family_index,
    };
    auto result = vkCreateCommandPool(
      core->device,
      &create_info,
      core->allocator,
      &data->command_pool
    );
    assert(result == VK_SUCCESS);
  }

   lib::gpu_signal::associate(
    gpu_signal_support.ptr,
    signal.ptr,
    core->device,
    data->semaphore_finished,
    1
  );

  lib::gfx::multi_alloc::init(
    &data->staging_multi_alloc,
    {
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_texture.width *
               data->the_texture.height *
               data->texel_size // engine::texture::ALBEDO_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->staging_buffer,
      },
      /*
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_normal.width *
               data->the_normal.height *
               engine::texture::NORMAL_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->texture_staging_normal,
      },
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_romeao.width *
               data->the_romeao.height *
               engine::texture::ROMEAO_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->texture_staging_romeao,
      }
      */
    },
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  lib::gfx::multi_alloc::init(
    &data->texture_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = data->texture_format, // engine::texture::ALBEDO_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_texture.width),
            .height = uint32_t(data->the_texture.height),
            .depth = 1,
          },
          .mipLevels = data->the_texture.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  /*
  lib::gfx::multi_alloc::init(
    &data->texture_normal_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = engine::texture::NORMAL_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_normal.width),
            .height = uint32_t(data->the_normal.height),
            .depth = 1,
          },
          .mipLevels = data->the_normal.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_normal_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  lib::gfx::multi_alloc::init(
    &data->texture_romeao_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = engine::texture::ROMEAO_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_romeao.width),
            .height = uint32_t(data->the_romeao.height),
            .depth = 1,
          },
          .mipLevels = data->the_romeao.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_romeao_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );
  */

  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = data->texture_format, // engine::texture::ALBEDO_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_texture.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_item.data.view
    );
    assert(result == VK_SUCCESS);
  }
  /*
  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_normal_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = engine::texture::NORMAL_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_normal.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_normal_item.data.view
    );
    assert(result == VK_SUCCESS);
  }
  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_romeao_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = engine::texture::ROMEAO_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_romeao.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_romeao_item.data.view
    );
    assert(result == VK_SUCCESS);
  }
  */

  VkCommandBuffer cmd;
  { ZoneScopedN("allocate_command_buffer");
    VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = data->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    {
      auto result = vkAllocateCommandBuffers(
        core->device,
        &alloc_info,
        &cmd
      );
      assert(result == VK_SUCCESS);
    }
  }

  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }

  engine::texture::prepare(
    data->texel_size,
    &data->the_texture,
    &data->texture_item.data,
    &data->staging_buffer,
    core.ptr,
    cmd,
    *queue_work
  );

  /*
  engine::texture::prepare(
    &data->the_normal,
    &data->texture_normal_item.data,
    &data->texture_staging_normal,
    core,
    cmd,
    queue_work
  );

  engine::texture::prepare(
    &data->the_romeao,
    &data->texture_romeao_item.data,
    &data->texture_staging_romeao,
    core,
    cmd,
    queue_work
  );
  */

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }

  { // submit
    uint64_t one = 1;
    auto timeline_info = VkTimelineSemaphoreSubmitInfo {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .signalSemaphoreValueCount = 1,
      .pSignalSemaphoreValues = &one,
    };
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &data->semaphore_finished,
    };
    auto result = vkQueueSubmit(
      *queue_work,
      1,
      &submit_info,
      VK_NULL_HANDLE
    );
    assert(result == VK_SUCCESS);
  }
}

void _load_cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::Vulkan::Core> core,
  Own<LoadData> data 
) {
  ZoneScoped;

  // can do this earlier in a separate task
  stbi_image_free(data->the_texture.data);

  lib::gfx::multi_alloc::deinit(
    &data->staging_multi_alloc,
    core->device,
    core->allocator
  );

  vkDestroyCommandPool(
   core->device,
   data->command_pool,
   core->allocator
  );

  vkDestroySemaphore(
   core->device,
   data->semaphore_finished,
   core->allocator
  );
}

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures,
  Own<LoadData> data
) {
  textures->items.insert({ data->texture_id, data->texture_item });
  auto meta = &meta_textures->items.at(data->texture_id);
  meta->status = SessionData::MetaTextures::Item::Status::Ready;
  meta->will_have_loaded = nullptr;

  delete data.ptr;
}

size_t get_texel_size(VkFormat format) {
  switch (format) {
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM: {
      return 4;
    }
    default: {
      // unsupported
      assert(false);
      return 0;
    }
  }
}

lib::Task *load(
  std::string path,
  VkFormat format,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaTextures> meta_textures,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_texture_id
) {
  auto it = meta_textures->by_key.find({ path, format });
  if (it != meta_textures->by_key.end()) {
    auto texture_id = it->second;
    *out_texture_id = texture_id;

    auto meta = &meta_textures->items.at(texture_id);
    meta->ref_count++;

    if (meta->status == SessionData::MetaTextures::Item::Status::Loading) {
      assert(meta->will_have_loaded != nullptr);
      return meta->will_have_loaded;
    }

    if (meta->status == SessionData::MetaTextures::Item::Status::Ready) {
      return nullptr;
    }

    assert(false);
  }

  auto texture_id = lib::guid::next(guid_counter.ptr);
  *out_texture_id = texture_id;

  SessionData::MetaTextures::Item meta = {
    .ref_count = 1, 
    .status = SessionData::MetaTextures::Item::Status::Loading,
    .path = path,
  };
  meta_textures->items.insert({ texture_id, meta });

  auto data = new LoadData {
    .texture_id = texture_id,
    .path = meta_textures->items.at(texture_id).path.c_str(),
    .texel_size = get_texel_size(format),
    .texture_format = format,
    // ^ this pointer will be valid for the tasks,
    // no need to copy the string
  };

  auto signal_init_image = lib::task::create_external_signal();
  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto task_init_image = lib::task::create(
    defer,
    task::create(
      _load_init_image,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &session->vulkan.queue_work,
      signal_init_image,
      data
    )
  );
  auto task_cleanup = lib::task::create(
    defer,
    task::create(
      _load_cleanup,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = lib::task::create(
    defer,
    lib::task::create(
      _load_finish,
      &session->vulkan.textures,
      &session->meta_textures,
      data
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_image,
    task_cleanup,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_image },
    { signal_init_image, task_cleanup },
    { task_cleanup, task_finish },
    // @Note: we don't strictly need cleanup to finish, but otherwise
    // we need an extra unfinished yarn for cleanup, which seems a bit rich.
  });

  return task_finish;
}

void deref(
  lib::GUID texture_id,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures
) {
  auto meta = &meta_textures->items.at(texture_id);
  assert(meta->ref_count > 0);
  meta->ref_count--;
  if (meta->ref_count == 0) {
    auto texture = &textures->items.at(texture_id);
    lib::gfx::multi_alloc::deinit(
      &texture->data.multi_alloc,
      core->device,
      core->allocator
    );

    vkDestroyImageView(
      core->device,
      texture->data.view,
      core->allocator
    );

    meta_textures->by_key.erase({ meta->path, meta->format });
    meta_textures->items.erase(texture_id);
    textures->items.erase(texture_id);
  }
}

} // namespace
