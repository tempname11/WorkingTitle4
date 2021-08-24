#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/gfx/allocator.hxx>
#include "uploader.data.hxx"
#include "session.hxx"

namespace engine::uploader {

void init(
  Uploader *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceMemoryProperties *properties,
  uint32_t transfer_queue_family_index,
  size_t size_region
);

void deinit(
  Uploader *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
);

struct PrepareResult {
  ID id;
  void *mem;
  size_t data_size;
};

PrepareResult prepare_buffer(
  Ref<Uploader> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkBufferCreateInfo *info
);

PrepareResult prepare_image(
  Ref<Uploader> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkImageCreateInfo *info
);

void upload_buffer(
  lib::task::ContextBase *ctx,
  lib::Task *signal,
  Ref<Uploader> it,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Use<SessionData::GPU_SignalSupport> gpu_signal_support,
  Own<VkQueue> queue_work,
  ID id
);

void upload_image(
  lib::task::ContextBase *ctx,
  lib::Task *signal,
  Ref<Uploader> it,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Use<SessionData::GPU_SignalSupport> gpu_signal_support,
  Own<VkQueue> queue_work,
  VkImageLayout final_layout,
  ID id
);

void destroy_buffer(
  Ref<Uploader> it,
  Use<SessionData::Vulkan::Core> core,
  ID id
);

void destroy_image(
  Ref<Uploader> it,
  Use<SessionData::Vulkan::Core> core,
  ID id
);

VkBuffer get_buffer(
  Ref<Uploader> it,
  ID id
);

std::pair<VkImage, VkImageView> get_image(
  Ref<Uploader> it,
  ID id
);

} // namespace