#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::datum::probe_offsets {

struct SData {
  lib::gfx::allocator::Image image;
  VkImageView view;
};

} // namespace