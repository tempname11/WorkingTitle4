#include "task.hxx"

void rendering_frame_example_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<RenderingData::Example> example_r,
  usage::Some<SessionData::Vulkan::Example> example_s,
  usage::Full<ExampleData> data
) {
  ZoneScoped;
  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  auto r = example_r.ptr;
  auto s = example_s.ptr;
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
  { TracyVkZone(core->tracy_context, cmd, "example_render");
    VkClearValue clear_values[] = {
      {0.0f, 0.0f, 0.0f, 0.0f},
      {1.0f, 0.0f},
    };
    VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = r->render_pass,
      .framebuffer = r->framebuffers[frame_info->inflight_index],
      .renderArea = {
        .offset = {0, 0},
        .extent = swapchain_description->image_extent,
      },
      .clearValueCount = sizeof(clear_values) / sizeof(*clear_values),
      .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r->pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &s->vertex_stake.buffer, &offset);
    uint32_t dynamicOffsets[2] = {
      frame_info->inflight_index * r->total_ubo_aligned_size,
      frame_info->inflight_index * r->total_ubo_aligned_size + r->vs_ubo_aligned_size,
    };
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
      s->pipeline_layout,
      0, 1, &s->descriptor_set,
      2, dynamicOffsets
    );
    vkCmdDraw(cmd, s->triangle_count * 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd);
  }
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}
