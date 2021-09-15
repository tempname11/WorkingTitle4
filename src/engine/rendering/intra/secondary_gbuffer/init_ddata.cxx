#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering/image_formats.hxx>
#include "constants.hxx"
#include "data.hxx"

namespace engine::rendering::intra::secondary_gbuffer {

void init_ddata(
  DData *out,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Use<SessionData::Vulkan::Core> core
) {
  ZoneScoped;

  std::vector<lib::gfx::allocator::Image> channel0_images;
  {
    channel0_images.resize(swapchain_description->image_count);

    VkImageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = GBUFFER_CHANNEL0_FORMAT,
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
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    for (size_t i = 0; i < channel0_images.size(); i++) {
      channel0_images[i] = lib::gfx::allocator::create_image(
        allocator_dedicated,
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
    }
  }

  std::vector<lib::gfx::allocator::Image> channel1_images;
  {
    channel1_images.resize(swapchain_description->image_count);

    VkImageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = GBUFFER_CHANNEL1_FORMAT,
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
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    for (size_t i = 0; i < channel1_images.size(); i++) {
      channel1_images[i] = lib::gfx::allocator::create_image(
        allocator_dedicated,
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
    }
  }

  std::vector<lib::gfx::allocator::Image> channel2_images;
  {
    channel2_images.resize(swapchain_description->image_count);

    VkImageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = GBUFFER_CHANNEL2_FORMAT,
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
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    for (size_t i = 0; i < channel2_images.size(); i++) {
      channel2_images[i] = lib::gfx::allocator::create_image(
        allocator_dedicated,
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
    }
  }

  std::vector<VkImageView> channel0_views;
  {
    channel0_views.resize(swapchain_description->image_count);

    for (size_t i = 0; i < channel0_views.size(); i++) {
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = channel0_images[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL0_FORMAT,
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
          &channel0_views[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
  }

  std::vector<VkImageView> channel1_views;
  {
    channel1_views.resize(swapchain_description->image_count);

    for (size_t i = 0; i < channel1_views.size(); i++) {
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = channel1_images[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL1_FORMAT,
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
          &channel1_views[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
  }

  std::vector<VkImageView> channel2_views;
  {
    channel2_views.resize(swapchain_description->image_count);

    for (size_t i = 0; i < channel2_views.size(); i++) {
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = channel2_images[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL2_FORMAT,
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
          &channel2_views[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
  }

  *out = {
    .channel0_images = std::move(channel0_images),
    .channel1_images = std::move(channel1_images),
    .channel2_images = std::move(channel2_images),
    .channel0_views = std::move(channel0_views),
    .channel1_views = std::move(channel1_views),
    .channel2_views = std::move(channel2_views),
  };
}

} // namespace
