#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "constants.hxx"
#include "data.hxx"

namespace engine::datum::probe_irradiance {

void init_ddata(
  DData *out,
  Ref<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  lib::gfx::allocator::Image image;
  {
    VkImageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = FORMAT,
      .extent = {
        .width = WIDTH,
        .height = HEIGHT,
        .depth = 1,
      },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = (0
        | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    image = lib::gfx::allocator::create_image(
      allocator_dedicated,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  VkImageView view;
  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
    };
    {
      auto result = vkCreateImageView(
        core->device,
        &create_info,
        core->allocator,
        &view
      );
      assert(result == VK_SUCCESS);
    }
  }

  *out = {
    .image = image,
    .view = view,
  };
}

} // namespace
