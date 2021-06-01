#pragma once
#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>

namespace lib::gfx::utilities {

VkDeviceSize aligned_size(VkDeviceSize size, VkDeviceSize alignment);

uint32_t find_memory_type_index(
  VkPhysicalDeviceMemoryProperties *properties,
  VkMemoryRequirements requirements,
  VkMemoryPropertyFlags flags
);

glm::mat4 get_projection(float aspect_ratio);

uint32_t mip_levels(int width, int height);

} // namespace