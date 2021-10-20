#pragma once
#include <vector>
#include <mutex>
#include <vulkan/vulkan.h>

struct CommandPool1 {
  VkCommandPool pool;
  std::vector<VkCommandBuffer> used_buffers;
};

struct CommandPool2 {
  std::mutex mutex;
  std::vector<CommandPool1 *> available;
};

CommandPool1 *command_pool_2_borrow(CommandPool2 *pool2);
void command_pool_2_return(CommandPool2 *pool2, CommandPool1 *pool1);