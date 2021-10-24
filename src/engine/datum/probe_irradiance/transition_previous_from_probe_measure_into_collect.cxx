#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::datum::probe_irradiance {

void transition_previous_from_probe_measure_into_collect(
  Use<DData> it,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  auto inflight_index_prev = (
    (frame_info->inflight_index + swapchain_description->image_count - 1) %
    swapchain_description->image_count
  );
  {
    VkImageMemoryBarrier barriers[] = {
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = it->images[inflight_index_prev].image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      },
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      sizeof(barriers) / sizeof(*barriers),
      barriers
    );
  }
}

} // namespace
