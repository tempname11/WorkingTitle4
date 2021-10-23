#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::datum::probe_radiance {

struct DData {
  std::vector<lib::gfx::allocator::Image> images;
  std::vector<VkImageView> views;
};

} // namespace