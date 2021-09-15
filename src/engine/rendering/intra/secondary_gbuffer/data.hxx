#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::rendering::intra::secondary_gbuffer {

struct DData {
  std::vector<lib::gfx::allocator::Image> channel0_images;
  std::vector<lib::gfx::allocator::Image> channel1_images;
  std::vector<lib::gfx::allocator::Image> channel2_images;
  std::vector<VkImageView> channel0_views;
  std::vector<VkImageView> channel1_views;
  std::vector<VkImageView> channel2_views;
};

} // namespace