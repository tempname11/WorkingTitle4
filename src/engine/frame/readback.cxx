#include <src/lib/gfx/utilities.hxx>
#include "readback.hxx"

namespace engine::frame {

void readback(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Own<engine::display::Data::Presentation> presentation,
  Use<VkSemaphore> frame_finished_semaphore,
  Use<engine::display::Data::LBuffer> lbuffer,
  Use<engine::display::Data::Readback> readback_data,
  Use<engine::display::Data::CommandPools> command_pools,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::FrameInfo> frame_info
) {
  ZoneScoped;

  auto level_count = lib::gfx::utilities::mip_levels(
    swapchain_description->image_extent.width,
    swapchain_description->image_extent.height
  );

  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  auto pool1 = command_pool_2_borrow(pool2);
  VkCommandBuffer cmd;
  { // cmd
    VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool1->pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    auto result = vkAllocateCommandBuffers(
      core->device,
      &info,
      &cmd
    );
    assert(result == VK_SUCCESS);
    ZoneValue(uint64_t(cmd));
  }
  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }

  { ZoneScopedN("transition_before");
    VkImageMemoryBarrier barriers[] = {
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = lbuffer->stakes[frame_info->inflight_index].image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      },
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = lbuffer->stakes[frame_info->inflight_index].image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 1,
          .levelCount = level_count - 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      },
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      sizeof(barriers) / sizeof(*barriers),
      barriers
    );
  }

  { ZoneScopedN("mip_levels");
    auto w = int32_t(swapchain_description->image_extent.width);
    auto h = int32_t(swapchain_description->image_extent.height);

    for (uint32_t m = 0; m < level_count - 1; m++) {
      auto next_w = w > 1 ? w / 2 : 1;
      auto next_h = h > 1 ? h / 2 : 1;
      VkImageBlit blit = {
        .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = m,
          .layerCount = 1,
        },
        .srcOffsets = {
          {},
          { .x = w, .y = h, .z = 1, },
        },
        .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = m + 1,
          .layerCount = 1,
        },
        .dstOffsets = {
          {},
          { .x = next_w, .y = next_h, .z = 1, },
        },
      };
      vkCmdBlitImage(
        cmd,
        lbuffer->stakes[frame_info->inflight_index].image, VK_IMAGE_LAYOUT_GENERAL,
        lbuffer->stakes[frame_info->inflight_index].image, VK_IMAGE_LAYOUT_GENERAL,
        1, &blit,
        VK_FILTER_LINEAR
      );
      w = next_w;
      h = next_h;
    }
  }
  
  {
    VkBufferImageCopy region = {
      .imageSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = level_count - 1,
        .layerCount = 1,
      },
      .imageExtent = {
        .width = 1,
        .height = 1,
        .depth = 1,
      },
    };
    vkCmdCopyImageToBuffer(
      cmd,
      lbuffer->stakes[frame_info->inflight_index].image,
      VK_IMAGE_LAYOUT_GENERAL,
      readback_data->luminance_buffers[frame_info->inflight_index].buffer,
      1,
      &region
    );
  }

  { ZoneScopedN("transition_after");
    VkImageMemoryBarrier barriers[] = {
      {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = lbuffer->stakes[frame_info->inflight_index].image,
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
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // finalpass on next frame
      0,
      0, nullptr,
      0, nullptr,
      sizeof(barriers) / sizeof(*barriers),
      barriers
    );
  }

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  pool1->used_buffers.push_back(cmd);
  command_pool_2_return(pool2, pool1);

  { ZoneScopedN("submit");
    VkSemaphore wait_semaphores[] = {
      presentation->image_rendered[frame_info->inflight_index]
    };
    VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    };

    VkSemaphore signal_semaphores[] = {
      *frame_finished_semaphore,
    };
    uint64_t signal_values[] = {
      frame_info->timeline_semaphore_value,
    };
    auto timeline_info = VkTimelineSemaphoreSubmitInfo {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .signalSemaphoreValueCount = sizeof(signal_values) / sizeof(*signal_values),
      .pSignalSemaphoreValues = signal_values,
    };
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .waitSemaphoreCount = sizeof(wait_semaphores) / sizeof(*wait_semaphores),
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(*signal_semaphores),
      .pSignalSemaphores = signal_semaphores,
    };
    auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
    assert(result == VK_SUCCESS);
  }
}

} // namespace
