#include "task.hxx"

void record_draw_commands(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Example *example_s
) {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &example_s->vertex_stake.buffer, &offset);
  vkCmdDraw(cmd, example_s->triangle_count * 3, 1, 0, 0);
}

void record_prepass(
  VkCommandBuffer cmd,
  RenderingData::Example::Prepass *prepass,
  RenderingData::Example::Constants *constants,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  SessionData::Vulkan::Example *example_s
) {
  VkClearValue clear_values[] = {
    {1.0f, 0.0f},
  };
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = prepass->render_pass,
    .framebuffer = prepass->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
    .clearValueCount = sizeof(clear_values) / sizeof(*clear_values),
    .pClearValues = clear_values,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, prepass->pipeline);
  uint32_t dynamic_offsets[2] = {
    frame_info->inflight_index * constants->total_ubo_aligned_size,
    frame_info->inflight_index * constants->total_ubo_aligned_size + constants->vs_ubo_aligned_size,
  };
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    example_s->pipeline_layout,
    0, 1, &example_s->descriptor_set,
    2, dynamic_offsets
  );
  record_draw_commands(cmd, example_s);
  vkCmdEndRenderPass(cmd);
}

void record_gpass(
  VkCommandBuffer cmd,
  RenderingData::Example::GPass *gpass,
  RenderingData::Example::Constants *constants,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  SessionData::Vulkan::Example *example_s
) {
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = gpass->render_pass,
    .framebuffer = gpass->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
    .clearValueCount = 0,
    .pClearValues = nullptr,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gpass->pipeline);
  uint32_t dynamic_offsets[2] = {
    frame_info->inflight_index * constants->total_ubo_aligned_size,
    frame_info->inflight_index * constants->total_ubo_aligned_size + constants->vs_ubo_aligned_size,
  };
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    example_s->pipeline_layout,
    0, 1, &example_s->descriptor_set,
    2, dynamic_offsets
  );
  record_draw_commands(cmd, example_s);
  vkCmdEndRenderPass(cmd);
}

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
  { TracyVkZone(core->tracy_context, cmd, "example_prepass");
    record_prepass(
      cmd,
      &r->prepass,
      &r->constants,
      swapchain_description.ptr,
      frame_info.ptr,
      example_s.ptr
    );
  }
  { TracyVkZone(core->tracy_context, cmd, "example_gpass");
    record_gpass(
      cmd,
      &r->gpass,
      &r->constants,
      swapchain_description.ptr,
      frame_info.ptr,
      example_s.ptr
    );
  }
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}
