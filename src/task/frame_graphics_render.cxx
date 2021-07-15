#include "frame_graphics_render.hxx"

void record_geometry_draw_commands(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  DescriptorPool *descriptor_pool,
  SessionData::Vulkan::GPass* s_gpass,
  RenderList *render_list,
  SessionData::Vulkan::Textures* textures
) {
  VkDescriptorSet descriptor_set;
  { ZoneScoped("descriptor_set");
    {
      std::scoped_lock lock(descriptor_pool->mutex);
      VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool->pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &s_gpass->descriptor_set_layout_mesh,
      };
      auto result = vkAllocateDescriptorSets(
        core->device,
        &allocate_info,
        &descriptor_set
      );
      assert(result == VK_SUCCESS);
    }

    VkDescriptorImageInfo albedo_image_info = {
      .sampler = s_gpass->sampler_albedo,
      .imageView = textures->albedo.view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo normal_image_info = {
      .sampler = s_gpass->sampler_albedo, // same for now
      .imageView = textures->normal.view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo romeao_image_info = {
      .sampler = s_gpass->sampler_albedo, // same for now
      .imageView = textures->romeao.view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &albedo_image_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &normal_image_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &romeao_image_info,
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes),
      writes,
      0, nullptr
    );
  }
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_gpass->pipeline_layout,
    1, 1, &descriptor_set,
    0, nullptr
  );
  VkDeviceSize offset = 0;
  for (auto &item : render_list->items) {
    // @Note: should use item.transform here!
    vkCmdBindVertexBuffers(cmd, 0, 1, &item.mesh_buffer, &offset);
    vkCmdDraw(cmd, item.mesh_vertex_count, 1, 0, 0);
  }
}

void record_prepass(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  DescriptorPool *descriptor_pool,
  RenderingData::Prepass *prepass,
  RenderingData::GPass *gpass,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  SessionData::Vulkan::Prepass *s_prepass,
  SessionData::Vulkan::GPass *s_gpass,
  RenderList *render_list,
  SessionData::Vulkan::Textures* textures
) {
  VkClearValue clear_values[] = {
    {1.0f, 0.0f},
  };
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = s_prepass->render_pass,
    .framebuffer = prepass->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
    .clearValueCount = sizeof(clear_values) / sizeof(*clear_values),
    .pClearValues = clear_values,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_prepass->pipeline);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = float(swapchain_description->image_extent.width),
    .height = float(swapchain_description->image_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = swapchain_description->image_extent,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_gpass->pipeline_layout,
    0, 1, &gpass->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );
  record_geometry_draw_commands(
    cmd,
    core,
    descriptor_pool,
    s_gpass,
    render_list,
    textures
  );
  vkCmdEndRenderPass(cmd);
}

void record_gpass(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  DescriptorPool *descriptor_pool,
  RenderingData::GPass *gpass,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  SessionData::Vulkan::GPass *s_gpass,
  RenderList *render_list,
  SessionData::Vulkan::Textures *textures
) {
  VkClearValue clear_values[] = {
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
  };
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = s_gpass->render_pass,
    .framebuffer = gpass->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
    .clearValueCount = sizeof(clear_values) / sizeof(*clear_values),
    .pClearValues = clear_values,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_gpass->pipeline);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = float(swapchain_description->image_extent.width),
    .height = float(swapchain_description->image_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = swapchain_description->image_extent,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_gpass->pipeline_layout,
    0, 1, &gpass->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );
  record_geometry_draw_commands(
    cmd,
    core,
    descriptor_pool,
    s_gpass,
    render_list,
    textures
  );
  vkCmdEndRenderPass(cmd);
}

void record_lpass(
  VkCommandBuffer cmd,
  RenderingData::LPass *lpass,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  SessionData::Vulkan::LPass *s_lpass,
  SessionData::Vulkan::FullscreenQuad *fullscreen_quad
) {
  VkClearValue clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = s_lpass->render_pass,
    .framebuffer = lpass->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
    .clearValueCount = 1,
    .pClearValues = &clear_value,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_lpass->pipeline_sun);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = float(swapchain_description->image_extent.width),
    .height = float(swapchain_description->image_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = swapchain_description->image_extent,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_lpass->pipeline_layout,
    0, 1, &lpass->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_lpass->pipeline_layout,
    1, 1, &lpass->descriptor_sets_directional_light[frame_info->inflight_index],
    0, nullptr
  );
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &fullscreen_quad->vertex_stake.buffer, &offset);
  vkCmdDraw(cmd, fullscreen_quad->triangle_count * 3, 1, 0, 0);
  vkCmdEndRenderPass(cmd);
}

void record_finalpass(
  VkCommandBuffer cmd,
  RenderingData::Finalpass *finalpass,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::FrameInfo *frame_info,
  RenderingData::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan::Finalpass *s_finalpass
) {
  ZoneScoped;
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, s_finalpass->pipeline);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    s_finalpass->pipeline_layout,
    0, 1, &finalpass->descriptor_sets[frame_info->inflight_index],
    0, nullptr
  );
  vkCmdDispatch(cmd,
    swapchain_description->image_extent.width,
    swapchain_description->image_extent.height,
    1
  );
}

void record_barrier_before_prepass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::ZBuffer *zbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = zbuffer->stakes[frame_info->inflight_index].image,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    },
  };
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_prepass_gpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::ZBuffer *zbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = zbuffer->stakes[frame_info->inflight_index].image,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    },
  };
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_before_gpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::GBuffer *gbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel0_stakes[frame_info->inflight_index].image,
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
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel1_stakes[frame_info->inflight_index].image,
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
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel2_stakes[frame_info->inflight_index].image,
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
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_gpass_lpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::GBuffer *gbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel0_stakes[frame_info->inflight_index].image,
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
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel1_stakes[frame_info->inflight_index].image,
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
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = gbuffer->channel2_stakes[frame_info->inflight_index].image,
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
      .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = zbuffer->stakes[frame_info->inflight_index].image,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
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
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_before_lpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::LBuffer *lbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_before_finalpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::FinalImage *final_image
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = final_image->stakes[frame_info->inflight_index].image,
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
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_lpass_finalpass(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::LBuffer *lbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

void record_barrier_finalpass_imgui(
  VkCommandBuffer cmd,
  RenderingData::FrameInfo *frame_info,
  RenderingData::FinalImage *final_image
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = final_image->stakes[frame_info->inflight_index].image,
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
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers),
    barriers
  );
}

TASK_DECL {
  ZoneScoped;
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

  record_barrier_before_prepass(
    cmd,
    frame_info.ptr,
    zbuffer.ptr
  );

  auto descriptor_pool = &(*descriptor_pools)[frame_info->inflight_index];

  { TracyVkZone(core->tracy_context, cmd, "prepass");
    record_prepass(
      cmd,
      core.ptr,
      descriptor_pool,
      prepass.ptr,
      gpass.ptr,
      swapchain_description.ptr,
      frame_info.ptr,
      s_prepass.ptr,
      s_gpass.ptr,
      render_list.ptr,
      textures.ptr
    );
  }

  record_barrier_prepass_gpass(
    cmd,
    frame_info.ptr,
    zbuffer.ptr
  );

  record_barrier_before_gpass(
    cmd,
    frame_info.ptr,
    gbuffer.ptr
  );

  { TracyVkZone(core->tracy_context, cmd, "gpass");
    record_gpass(
      cmd,
      core.ptr,
      descriptor_pool,
      gpass.ptr,
      swapchain_description.ptr,
      frame_info.ptr,
      s_gpass.ptr,
      render_list.ptr,
      textures.ptr
    );
  }

  record_barrier_before_lpass(
    cmd,
    frame_info.ptr,
    lbuffer.ptr
  );

  record_barrier_gpass_lpass(
    cmd,
    frame_info.ptr,
    zbuffer.ptr,
    gbuffer.ptr
  );

  { TracyVkZone(core->tracy_context, cmd, "lpass");
    record_lpass(
      cmd,
      lpass.ptr,
      swapchain_description.ptr,
      frame_info.ptr,
      s_lpass.ptr,
      fullscreen_quad.ptr
    );
  }

  record_barrier_before_finalpass(
    cmd,
    frame_info.ptr,
    final_image.ptr
  );

  record_barrier_lpass_finalpass(
    cmd,
    frame_info.ptr,
    lbuffer.ptr
  );

  { TracyVkZone(core->tracy_context, cmd, "finalpass");
    record_finalpass(
      cmd,
      finalpass.ptr,
      swapchain_description.ptr,
      frame_info.ptr,
      lbuffer.ptr,
      final_image.ptr,
      s_finalpass.ptr
    );
  }

  // @Note: this relies on imgui cmd being sent next!
  record_barrier_finalpass_imgui(
    cmd,
    frame_info.ptr,
    final_image.ptr
  );

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}
