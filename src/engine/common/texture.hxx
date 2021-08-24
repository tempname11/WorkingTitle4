#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace engine::common::texture {
  const auto ALBEDO_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

  const auto NORMAL_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
  // SNORM doesn't work right, for some reason.

  const auto ROMEAO_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

  template<typename T>
  struct Data {
    T *data;
    int width;
    int height;
    int channels;
    uint32_t computed_mip_levels;
  };
}