#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/task/defer.hxx>
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

PrepareBufferResult prepare_buffer(
  Ref<Uploader> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkBufferCreateInfo *info
) {
  ZoneScoped;

  auto original_usage = info->usage;

  info->usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  auto buffer_host = lib::gfx::allocator::create_buffer(
    &it->allocator_host,
    device,
    allocator,
    properties,
    info
  );

  info->usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
        .size = info->size, // not mapping.size !
      },
    });
  }
  assert(id != 0);

  return {
    .id = id,
    .mem = mapping.mem,
    .mem_size = mapping.size,
  };
}

struct UploadData {
  ID id;
  VkCommandBuffer cmd;
  VkSemaphore semaphore;
};

void _upload_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Uploader> it,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
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

void _upload_submit(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Own<UploadData> data
) {
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

void upload(
  lib::task::ContextBase *ctx,
  lib::Task *signal,
  Ref<Uploader> it,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Use<SessionData::GPU_SignalSupport> gpu_signal_support,
  Own<VkQueue> queue_work,
  ID id
) {
  ZoneScoped;

  std::unique_lock lock(it->rw_mutex);
  auto buffer_data = &it->buffers.at(id);
  assert(buffer_data->status == Uploader::BufferData::Status::New);
  buffer_data->status = Uploader::BufferData::Status::Uploading;

  auto region = VkBufferCopy {
    .size = buffer_data->size,
  };

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
    }
  }

  auto info = VkCommandBufferBeginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  auto result = vkBeginCommandBuffer(cmd, &info);
  assert(result == VK_SUCCESS);

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

  auto task_finish = defer(
    lib::task::create(
      _upload_finish,
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
  Use<SessionData::Vulkan::Core> core,
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

VkBuffer get_buffer(
  Ref<Uploader> it,
  ID id
) {
  ZoneScoped;

  std::shared_lock lock(it->rw_mutex);
  assert(it->buffers.contains(id));
  return it->buffers.at(id).buffer_device.buffer;
}

} // namespace