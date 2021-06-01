#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace lib::gfx::multi_alloc {

struct StakeBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize offset;
  VkDeviceSize size;
};

struct StakeImage {
  VkImage image;
  VkDeviceMemory memory;
  VkDeviceSize offset;
  VkDeviceSize size;
};

struct Claim {
  union {
    struct { VkStructureType type; } structure;
    VkBufferCreateInfo buffer;
    VkImageCreateInfo image;
  } info;
  VkMemoryPropertyFlagBits memory_property_flags;
  StakeBuffer *p_stake_buffer;
  StakeImage *p_stake_image;
};

struct Instance {
  std::vector<VkBuffer> all_buffers;
  std::vector<VkImage> all_images;
  std::vector<VkDeviceMemory> all_memories;
};

void init(
  Instance *it,
  std::vector<Claim> &&claims,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkPhysicalDeviceMemoryProperties *memory_properties
);
void deinit(
  Instance *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
);

} // namespace