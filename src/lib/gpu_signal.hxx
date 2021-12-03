#pragma once
#include <thread>
#include <vulkan/vulkan.h>
#include "task.hxx"

namespace lib::gpu_signal {

struct Storage;

struct Support {
  Storage *storage;
  VkSemaphore new_entries_semaphore;
  std::thread thread;
};

// note: the returned pointer will be valid only until the semaphore is signaled.
// so the caller needs to guarantee all operations with it are done before the
// semaphore is signalled
lib::Task *create(
  Support *it,
  VkDevice device,
  VkSemaphore semaphore,
  uint64_t value
);

void associate(
  Support *it,
  lib::Task *external_signal,
  VkDevice device,
  VkSemaphore semaphore,
  uint64_t value
);

void init_support(
  Support *out,
  lib::task::Runner *tr,
  VkDevice device,
  const VkAllocationCallbacks *allocator
);

void deinit_support(
  Support *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
);

} // namespace