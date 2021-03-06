#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "constants.hxx"
#include "data.hxx"

namespace engine::datum::probe_attention {

void init_ddata(
  DData *out,
  Ref<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  std::vector<lib::gfx::allocator::Image> images;
  {
    images.resize(swapchain_description->image_count);

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
        | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    for (size_t i = 0; i < images.size(); i++) {
      images[i] = lib::gfx::allocator::create_image(
        allocator_dedicated,
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
    }
  }

  std::vector<VkImageView> views;
  {
    views.resize(swapchain_description->image_count);

    for (size_t i = 0; i < views.size(); i++) {
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = images[i].image,
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
          &views[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
  }

  *out = {
    .images = std::move(images),
    .views = std::move(views),
  };
}

} // namespace
