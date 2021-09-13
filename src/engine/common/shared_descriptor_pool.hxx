#pragma once
#include <mutex>
#include <vulkan/vulkan.h>

namespace engine::common {

struct SharedDescriptorPool {
  std::mutex mutex;
  VkDescriptorPool pool;
};

} // namespace
