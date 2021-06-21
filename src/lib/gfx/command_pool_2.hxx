#pragma once
#include <vector>
#include <mutex>
#include <vulkan/vulkan.h>

struct CommandPool2 {
  std::mutex mutex;
  std::vector<VkCommandPool> pools;
};

VkCommandPool command_pool_2_borrow(CommandPool2 *pool2);
void command_pool_2_return(CommandPool2 *pool2, VkCommandPool pool);