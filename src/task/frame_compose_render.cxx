#include "frame_compose_render.hxx"

TASK_DECL {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      return;
    }
  }
  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  VkCommandPool pool = command_pool_2_borrow(pool2);
  VkCommandBuffer cmd;
  { // cmd
    VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    auto result = vkAllocateCommandBuffers(
      core->device,
      &info,
      &cmd
    );
    assert(result == VK_SUCCESS);
  }
  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }
  { TracyVkZone(core->tracy_context, cmd, "compose_render");
    { ZoneScopedN("barriers_before");
      VkImageMemoryBarrier barriers[] = {
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .srcAccessMask = 0, // wait for nothing
          .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = presentation->swapchain_images[presentation->latest_image_index],
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          }
        },
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = final_image->stakes[frame_info->inflight_index].image,
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          }
        }
      };
      vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        sizeof(barriers) / sizeof(*barriers),
        barriers
      );
    }
    { // copy image
      auto region = VkImageCopy {
        .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
        },
        .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
        },
        .extent = {
          .width = swapchain_description->image_extent.width,
          .height = swapchain_description->image_extent.height,
          .depth = 1,
        },
      };
      vkCmdCopyImage(
        cmd,
        final_image->stakes[frame_info->inflight_index].image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        presentation->swapchain_images[presentation->latest_image_index],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
      );
    }
    { ZoneScopedN("barriers_after");
      // @Note:
      // https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
      // "A PRACTICAL BOTTOM_OF_PIPE EXAMPLE" suggests we can
      // use dstStageMask = BOTTOM_OF_PIPE, dstAccessMask = 0
      VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = 0, // see above
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = presentation->swapchain_images[presentation->latest_image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };
      vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // see above
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );
    }
  }
  TracyVkCollect(core->tracy_context, cmd);
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}
