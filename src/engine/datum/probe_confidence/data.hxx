#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::datum::probe_confidence {

struct SData {
  lib::gfx::allocator::Image image;
  VkImageView view;
};

} // namespace