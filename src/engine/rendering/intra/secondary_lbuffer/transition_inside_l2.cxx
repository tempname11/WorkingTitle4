#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::intra::secondary_lbuffer {

void transition_inside_l2(
  Use<DData> it,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  // So, this barrier would not be necessary with proper render pass usage.
  // :UseRenderPasses

  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = it->images[frame_info->inflight_index].image,
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
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

} // namespace
