#include <cassert>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include "utilities.hxx"

namespace lib::gfx::utilities {

VkDeviceSize aligned_size(VkDeviceSize size, VkDeviceSize alignment) {
  return ((alignment + size - 1) / alignment) * alignment;
}

uint32_t find_memory_type_index(
  VkPhysicalDeviceMemoryProperties *properties,
  VkMemoryRequirements requirements,
  VkMemoryPropertyFlags flags
) {
  uint32_t memory_type_index = (uint32_t) -1;
  for (uint32_t i = 0; i < properties->memoryTypeCount; i++) {
    if (true
      && (requirements.memoryTypeBits & (1 << i))
      && ((properties->memoryTypes[i].propertyFlags & flags) == flags)
    ) {
      memory_type_index = i;
    }
  }
  assert(memory_type_index != (uint32_t) -1);
  return memory_type_index;
}

glm::mat4 get_projection(float aspect_ratio) {
  return glm::perspective(
    glm::pi<float>() * 0.5f,
    aspect_ratio,
    0.1f, 100.f
  ) * glm::scale( // Vulkan convention -> OpenGL convention
    glm::mat4(1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f)
  ) * glm::mat4_cast( // MagicaVoxel convention -> Vulkan convention
    glm::angleAxis( 
      -glm::half_pi<float>(),
      glm::vec3(1.0f, 0.0f, 0.0f)
    )
  );
}

uint32_t mip_levels(int width, int height) {
  return log2(
    std::max(width, height)
  ) + 1;
}

} // namespace