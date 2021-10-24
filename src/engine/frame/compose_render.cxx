#include <stb_image_write.h>
#include <src/lib/gfx/utilities.hxx>
#include "compose_render.hxx"

namespace engine::frame {

#ifdef ENGINE_DEVELOPER
  struct WriteScreenshotData {
    lib::gfx::allocator::Buffer buffer;
    lib::gfx::allocator::HostMapping mapping;
    std::string path;
    VkFormat format;
    int w;
    int h;
  };

  void _write_screenshot(
    lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
    Ref<engine::session::Data> session,
    Ref<engine::session::Vulkan::Core> core,
    Own<WriteScreenshotData> data
  ) {
    ZoneScoped;

    assert(data->format == VK_FORMAT_B8G8R8A8_SRGB);
    for (size_t i = 0; i < data->w * data->h; i++) {
      auto ptr = (uint8_t *) data->mapping.mem;
      // swap R <-> B
      auto r = ptr[4 * i + 0];
      ptr[4 * i + 0] = ptr[4 * i + 2];
      ptr[4 * i + 2] = r;
    }

    auto result = stbi_write_bmp(
      data->path.c_str(),
      data->w,
      data->h,
      4,
      data->mapping.mem
      //data->w * 4
    );

    assert(result != 0);

    lib::gfx::allocator::destroy_buffer(
      &session->vulkan.uploader.allocator_host,
      core->device,
      core->allocator,
      data->buffer
    );

    delete data.ptr;

    lib::lifetime::deref(&session->lifetime, ctx->runner);
  }
#endif

void compose_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display,
  Ref<engine::session::Vulkan::Core> core,
  Use<engine::display::Data::Presentation> presentation,
  Use<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::FinalImage> final_image,
  Own<engine::misc::ComposeData> data
) {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      data->cmd = VK_NULL_HANDLE;
      return;
    }
  }

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

    #ifdef ENGINE_DEVELOPER
    if (frame_info->directives.should_capture_screenshot) { // screenshot
      VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = (lib::gfx::utilities::get_format_byte_size(swapchain_description->image_format)
          * swapchain_description->image_extent.width
          * swapchain_description->image_extent.height
        ),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      };
      auto buffer = lib::gfx::allocator::create_buffer(
        &session->vulkan.uploader.allocator_host, // @Cleanup
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
      auto mapping = lib::gfx::allocator::get_host_mapping(
        &session->vulkan.uploader.allocator_host, // @Cleanup
        buffer.id
      );
      auto region = VkBufferImageCopy {
        .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
        },
        .imageExtent = {
          .width = swapchain_description->image_extent.width,
          .height = swapchain_description->image_extent.height,
          .depth = 1,
        },
      };
      vkCmdCopyImageToBuffer(
        cmd,
        final_image->stakes[frame_info->inflight_index].image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer.buffer,
        1,
        &region
      );

      auto write_data = new WriteScreenshotData {
        .buffer = buffer,
        .mapping = mapping,
        .path = std::move(frame_info->directives.screenshot_path),
        .format = swapchain_description->image_format,
        .w = int(swapchain_description->image_extent.width),
        .h = int(swapchain_description->image_extent.height),
          
      };

      lib::lifetime::ref(&session->lifetime);

      auto task_write_screenshot = lib::task::create(
        _write_screenshot,
        session.ptr,
        core.ptr,
        write_data
      );

      ctx->new_tasks.insert(ctx->new_tasks.end(), {
        task_write_screenshot,
      });

      auto signal = session->inflight_gpu.signals[frame_info->inflight_index];
      assert(signal != nullptr);

      ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
        { signal, task_write_screenshot },
      });
    }
    #endif

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
  pool1->used_buffers.push_back(cmd);
  command_pool_2_return(pool2, pool1);
  data->cmd = cmd;
}

} // namespace
