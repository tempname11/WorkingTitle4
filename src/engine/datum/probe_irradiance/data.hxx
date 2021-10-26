#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::datum::probe_irradiance {

struct DData {
  lib::gfx::allocator::Image image;
  VkImageView view;
};

} // namespace