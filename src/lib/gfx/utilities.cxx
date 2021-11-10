#include <cassert>
#include <random>
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
    0.1f, 10000.f
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

static thread_local std::random_device rd;
static thread_local std::mt19937 mt(rd());

float rand_unit() {
  return std::uniform_real_distribution(-1.0, 1.0)(mt);
}

glm::vec3 get_random_unit_vector() {
  auto point = glm::vec3(
    rand_unit(),
    rand_unit(),
    rand_unit()
  );
  if (glm::length(point) > 1.0) {
    return get_random_unit_vector(); // try another
  }
  return glm::normalize(point);
}

glm::mat3 get_random_rotation() {
  auto u = get_random_unit_vector();
  auto v_ = get_random_unit_vector();
  if (glm::dot(u, v_) > 0.9) {
    return get_random_rotation(); // try again
  }
  auto w = glm::normalize(glm::cross(u, v_));
  auto v = glm::cross(w, u);
  return glm::mat3(u, v, w);
}

size_t get_format_byte_size(VkFormat format) {
  switch (format) {
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM: {
      return 4;
    }
    default: {
      // If this asserts, time to add new cases!
      // Not sure if Vulkan API can do this for us instead?
      assert("Unknown format" && false);
      return 0;
    }
  }
}

} // namespace