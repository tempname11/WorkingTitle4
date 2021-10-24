#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/defer.hxx>
#include "uploader.hxx"

namespace engine::uploader {

void init(
  Uploader *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceMemoryProperties *properties,
  uint32_t transfer_queue_family_index,
  size_t size_region
) {
  ZoneScoped;

  lib::gfx::allocator::init(
    &it->allocator_host,
    properties,
    VkMemoryPropertyFlagBits(0
      | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    ),
    size_region,
    "uploader.allocator_host"
  );

  lib::gfx::allocator::init(
    &it->allocator_device,
    properties,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    size_region,
    "uploader.allocator_device"
  );

  { ZoneScopedN("command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = transfer_queue_family_index,
    };
    auto result = vkCreateCommandPool(
      device,
      &create_info,
      allocator,
      &it->command_pool
    );
    assert(result == VK_SUCCESS);
  }
}

void deinit(
  Uploader *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  ZoneScoped;

  lib::gfx::allocator::deinit(
    &it->allocator_host,
    device,
    allocator
  );

  lib::gfx::allocator::deinit(
    &it->allocator_device,
    device,
    allocator
  );

  vkDestroyCommandPool(
    device,
    it->command_pool,
    allocator
  );
}

PrepareResult prepare_buffer(
  Ref<Uploader> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkBufferCreateInfo *info
) {
  ZoneScoped;

  auto original_usage = info->usage;

  info->usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  auto buffer_host = lib::gfx::allocator::create_buffer(
    &it->allocator_host,
    device,
    allocator,
    properties,
    info
  );

  info->usage = (original_usage
    | VK_BUFFER_USAGE_TRANSFER_DST_BIT
  );
  auto buffer_device = lib::gfx::allocator::create_buffer(
    &it->allocator_device,
    device,
    allocator,
    properties,
    info
  );

  info->usage = original_usage;

  auto mapping = lib::gfx::allocator::get_host_mapping(
    &it->allocator_host,
    buffer_host.id
  );

  ID id = 0;
  {
    std::unique_lock lock(it->rw_mutex);
    it->last_id++;
    id = it->last_id;
    it->buffers.insert({
      id,
      Uploader::BufferData {
        .status = Uploader::BufferData::Status::New,
        .buffer_host = buffer_host,
        .buffer_device = buffer_device,
        .size = info->size,
      },
    });
  }
  assert(id != 0);

  return {
    .id = id,
    .mem = mapping.mem,
    .data_size = info->size,
  };
}

PrepareResult prepare_image(
  Ref<Uploader> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkImageCreateInfo *info
) {
  ZoneScoped;

  auto original_usage = info->usage;

  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = VkDeviceSize(
      info->extent.width *
      info->extent.height *
      lib::gfx::utilities::get_format_byte_size(info->format)
    ),
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  auto buffer_host = lib::gfx::allocator::create_buffer(
    &it->allocator_host,
    device,
    allocator,
    properties,
    &buffer_info
  );

  info->usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  info->usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  auto image_device = lib::gfx::allocator::create_image(
    &it->allocator_device,
    device,
    allocator,
    properties,
    info
  );

  info->usage = original_usage;

  VkImageView image_view;
  { // image view
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image_device.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = info->format,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = info->mipLevels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      device,
      &create_info,
      allocator,
      &image_view
    );
    assert(result == VK_SUCCESS);
  }

  auto mapping = lib::gfx::allocator::get_host_mapping(
    &it->allocator_host,
    buffer_host.id
  );

  ID id = 0;
  {
    std::unique_lock lock(it->rw_mutex);
    it->last_id++;
    id = it->last_id;
    it->images.insert({
      id,
      Uploader::ImageData {
        .status = Uploader::ImageData::Status::New,
        .buffer_host = buffer_host,
        .image_device = image_device,
        .image_view = image_view,
        .width = info->extent.width,
        .height = info->extent.height,
        .computed_mip_levels = info->mipLevels,
      },
    });
  }
  assert(id != 0);

  return {
    .id = id,
    .mem = mapping.mem,
    .data_size = mapping.size,
  };
}

struct UploadData {
  ID id;
  VkCommandBuffer cmd;
  VkSemaphore semaphore;
};

void _upload_buffer_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Uploader> it,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Own<UploadData> data
) {
  ZoneScoped;

  vkDestroySemaphore(
    core->device,
    data->semaphore,
    core->allocator
  );

  {
    std::unique_lock lock(it->rw_mutex);

    vkFreeCommandBuffers(
      core->device,
      it->command_pool,
      1,
      &data->cmd
    );

    auto buffer_data = &it->buffers.at(data->id);
    buffer_data->status = Uploader::BufferData::Status::Ready;

    lib::gfx::allocator::destroy_buffer(
      &it->allocator_host,
      core->device,
      core->allocator,
      buffer_data->buffer_host
    );
    buffer_data->buffer_host = {};
  }

  delete data.ptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

void _upload_image_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Uploader> it,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Own<UploadData> data
) {
  ZoneScoped;

  vkDestroySemaphore(
    core->device,
    data->semaphore,
    core->allocator
  );

  {
    std::unique_lock lock(it->rw_mutex);

    vkFreeCommandBuffers(
      core->device,
      it->command_pool,
      1,
      &data->cmd
    );

    auto image_data = &it->images.at(data->id);
    image_data->status = Uploader::ImageData::Status::Ready;

    lib::gfx::allocator::destroy_buffer(
      &it->allocator_host,
      core->device,
      core->allocator,
      image_data->buffer_host
    );
    image_data->buffer_host = {};
  }

  delete data.ptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

void _upload_submit(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Own<UploadData> data
) {
  ZoneScoped;

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
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &data->semaphore,
  };
  auto result = vkQueueSubmit(
    *queue_work,
    1,
    &submit_info,
    VK_NULL_HANDLE
  );
  assert(result == VK_SUCCESS);
}

void upload_buffer(
  lib::task::ContextBase *ctx,
  lib::Task *signal,
  Ref<Uploader> it,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Use<engine::session::Data::GPU_SignalSupport> gpu_signal_support,
  Own<VkQueue> queue_work,
  ID id
) {
  ZoneScoped;

  std::unique_lock lock(it->rw_mutex);
  auto buffer_data = &it->buffers.at(id);
  assert(buffer_data->status == Uploader::BufferData::Status::New);
  buffer_data->status = Uploader::BufferData::Status::Uploading;

  VkSemaphore semaphore;
  { ZoneScopedN("semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      core->device,
      &create_info,
      core->allocator,
      &semaphore
    );
    assert(result == VK_SUCCESS);
  }

  // create cmd
  VkCommandBuffer cmd;
  { ZoneScopedN("allocate_command_buffer");
    VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = it->command_pool,
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
      ZoneValue(uint64_t(cmd));

    }
  }

  {
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }

  VkBufferCopy region = {
    .size = buffer_data->size,
  };
  vkCmdCopyBuffer(
    cmd,
    buffer_data->buffer_host.buffer,
    buffer_data->buffer_device.buffer,
    1,
    &region
  );

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }

  lib::gpu_signal::associate(
    gpu_signal_support.ptr,
    signal,
    core->device,
    semaphore,
    1
  );

  auto data = new UploadData {
    .id = id,
    .cmd = cmd,
    .semaphore = semaphore,
  };

  auto task_submit = lib::task::create(
    _upload_submit,
    &session->vulkan.queue_work,
    data
  );

  // @Note: passing `it` to the task is bit fishy,
  // because it relies on it having at least the lifetime of `session`.
  // Which is how it is currently used, but there's no rule or convention
  // preventing it to be used separately.
  lib::lifetime::ref(&session->lifetime);
  auto task_finish = lib::defer(
    lib::task::create(
      _upload_buffer_finish,
      it.ptr,
      session.ptr,
      &session->vulkan.core,
      data
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_submit,
    task_finish.first,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { signal, task_finish.first },
  });
}

void upload_image(
  lib::task::ContextBase *ctx,
  lib::Task *signal,
  Ref<Uploader> it,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Use<engine::session::Data::GPU_SignalSupport> gpu_signal_support,
  Own<VkQueue> queue_work,
  VkImageLayout final_layout,
  ID id
) {
  ZoneScoped;

  std::unique_lock lock(it->rw_mutex);
  auto image_data = &it->images.at(id);
  assert(image_data->status == Uploader::ImageData::Status::New);
  image_data->status = Uploader::ImageData::Status::Uploading;

  VkSemaphore semaphore;
  { ZoneScopedN("semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      core->device,
      &create_info,
      core->allocator,
      &semaphore
    );
    assert(result == VK_SUCCESS);
  }

  // create cmd
  VkCommandBuffer cmd;
  { ZoneScopedN("allocate_command_buffer");
    VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = it->command_pool,
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
      ZoneValue(uint64_t(cmd));
    }
  }

  auto info = VkCommandBufferBeginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  auto result = vkBeginCommandBuffer(cmd, &info);
  assert(result == VK_SUCCESS);

  { ZoneScopedN("barriers_before");
    VkImageMemoryBarrier barriers[] = {
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0, // wait for nothing
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image_data->image_device.image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      },
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      sizeof(barriers) / sizeof(*barriers),
      barriers
    );
  }

  VkBufferImageCopy region = {
    .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .layerCount = 1,
    },
    .imageExtent = {
      .width = uint32_t(image_data->width),
      .height = uint32_t(image_data->height),
      .depth = 1,
    },
  };

  vkCmdCopyBufferToImage(
    cmd,
    image_data->buffer_host.buffer,
    image_data->image_device.image,
    VK_IMAGE_LAYOUT_GENERAL,
    1,
    &region
  );

  { // generate mip levels
    auto w = int32_t(image_data->width);
    auto h = int32_t(image_data->height);

    for (uint32_t m = 0; m < image_data->computed_mip_levels - 1; m++) {
      auto next_w = w > 1 ? w / 2 : 1;
      auto next_h = h > 1 ? h / 2 : 1;
      VkImageBlit blit = {
        .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = m,
          .layerCount = 1,
        },
        .srcOffsets = {
          {},
          { .x = w, .y = h, .z = 1, },
        },
        .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = m + 1,
          .layerCount = 1,
        },
        .dstOffsets = {
          {},
          { .x = next_w, .y = next_h, .z = 1, },
        },
      };
      vkCmdBlitImage(
        cmd,
        image_data->image_device.image, VK_IMAGE_LAYOUT_GENERAL,
        image_data->image_device.image, VK_IMAGE_LAYOUT_GENERAL,
        1, &blit,
        VK_FILTER_LINEAR
      );
      w = next_w;
      h = next_h;
    }
  }

  { ZoneScopedN("barriers_after");
    VkImageMemoryBarrier barriers[] = {
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = final_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image_data->image_device.image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = image_data->computed_mip_levels,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      },
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      0,
      0, nullptr,
      0, nullptr,
      sizeof(barriers) / sizeof(*barriers),
      barriers
    );
  }

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }

  lib::gpu_signal::associate(
    gpu_signal_support.ptr,
    signal,
    core->device,
    semaphore,
    1
  );

  auto data = new UploadData {
    .id = id,
    .cmd = cmd,
    .semaphore = semaphore,
  };

  auto task_submit = lib::task::create(
    _upload_submit,
    &session->vulkan.queue_work,
    data
  );

  auto task_finish = lib::defer(
    lib::task::create(
      _upload_image_finish,
      it.ptr,
      session.ptr,
      &session->vulkan.core,
      data
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_submit,
    task_finish.first,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { signal, task_finish.first },
  });

  lib::lifetime::ref(&session->lifetime);
}

void destroy_buffer(
  Ref<Uploader> it,
  Use<engine::session::Vulkan::Core> core,
  ID id
) {
  ZoneScoped;

  lib::gfx::allocator::Buffer buffer_device;

  {
    std::unique_lock lock(it->rw_mutex);
    auto buffer_data = &it->buffers.at(id);
    assert(buffer_data->status == Uploader::BufferData::Status::Ready);
    // @Incomplete should wait for Uploaded...

    buffer_device = buffer_data->buffer_device;
    assert(buffer_data->buffer_host.id == 0);
    it->buffers.erase(id);
  }

  lib::gfx::allocator::destroy_buffer(
    &it->allocator_device,
    core->device,
    core->allocator,
    buffer_device
  );
}

void destroy_image(
  Ref<Uploader> it,
  Use<engine::session::Vulkan::Core> core,
  ID id
) {
  ZoneScoped;

  VkImageView image_view;
  lib::gfx::allocator::Image image_device;

  {
    std::unique_lock lock(it->rw_mutex);
    auto image_data = &it->images.at(id);
    assert(image_data->status == Uploader::ImageData::Status::Ready);
    // @Incomplete should wait for Uploaded...

    image_view = image_data->image_view;
    image_device = image_data->image_device;
    assert(image_data->buffer_host.id == 0);
    it->images.erase(id);
  }

  vkDestroyImageView(
    core->device,
    image_view,
    core->allocator
  );

  lib::gfx::allocator::destroy_image(
    &it->allocator_device,
    core->device,
    core->allocator,
    image_device
  );
}

std::pair<VkBuffer, VkDeviceAddress> get_buffer(
  Ref<Uploader> it,
  ID id
) {
  ZoneScoped;

  std::shared_lock lock(it->rw_mutex);
  assert(it->buffers.contains(id));
  auto item = &it->buffers.at(id);
  return {
    item->buffer_device.buffer,
    item->buffer_device.buffer_address,
  };
}

std::pair<VkImage, VkImageView> get_image(
  Ref<Uploader> it,
  ID id
) {
  ZoneScoped;

  std::shared_lock lock(it->rw_mutex);
  assert(it->images.contains(id));
  auto item = &it->images.at(id);
  return { item->image_device.image, item->image_view };
}

} // namespace