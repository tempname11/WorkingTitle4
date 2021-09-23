#include <src/task/defer.hxx>
#include <src/engine/common/shared_descriptor_pool.hxx>
#include <src/engine/rendering/pass/secondary_geometry.hxx>
#include <src/engine/rendering/pass/directional_light_secondary.hxx>
#include <src/engine/rendering/pass/probe_maps_update.hxx>
#include <src/engine/rendering/pass/indirect_light.hxx>
#include <src/engine/rendering/intra/secondary_zbuffer.hxx>
#include <src/engine/rendering/intra/secondary_gbuffer.hxx>
#include <src/engine/rendering/intra/secondary_lbuffer.hxx>
#include <src/engine/rendering/intra/probe_light_map.hxx>
#include "frame_graphics_render.hxx"

void record_geometry_draw_commands(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  engine::common::SharedDescriptorPool *descriptor_pool,
  SessionData::Vulkan::GPass* s_gpass,
  engine::misc::RenderList *render_list
) {
  if (render_list->items.size() == 0) {
    return;
  }

  std::vector<VkDescriptorSet> descriptor_sets(render_list->items.size());
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts(render_list->items.size());
  for (size_t i = 0; i < render_list->items.size(); i++) {
    descriptor_set_layouts[i] = s_gpass->descriptor_set_layout_mesh;
  }
  { ZoneScoped("descriptor_sets");
    {
      std::scoped_lock lock(descriptor_pool->mutex);
      VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool->pool,
        .descriptorSetCount = checked_integer_cast<uint32_t>(render_list->items.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
      };
      auto result = vkAllocateDescriptorSets(
        core->device,
        &allocate_info,
        descriptor_sets.data()
      );
      assert(result == VK_SUCCESS);
    }
  }

  for (size_t i = 0; i < render_list->items.size(); i++) {
    auto &item = render_list->items[i];
    auto descriptor_set = descriptor_sets[i];
    VkDescriptorImageInfo albedo_image_info = {
      .sampler = s_gpass->sampler_albedo,
      .imageView = item.texture_albedo_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo normal_image_info = {
      .sampler = s_gpass->sampler_albedo, // same for now
      .imageView = item.texture_normal_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo romeao_image_info = {
      .sampler = s_gpass->sampler_albedo, // same for now
      .imageView = item.texture_romeao_view,
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
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
      s_gpass->pipeline_layout,
      1, 1, &descriptor_set,
      0, nullptr
    );
    VkDeviceSize vertices_offset = item.mesh_buffer_offset_vertices;
    vkCmdBindVertexBuffers(cmd, 0, 1, &item.mesh_buffer, &vertices_offset);
    vkCmdBindIndexBuffer(cmd, item.mesh_buffer, item.mesh_buffer_offset_indices, VK_INDEX_TYPE_UINT16);
    vkCmdPushConstants(
      cmd,
      s_gpass->pipeline_layout,
      VK_SHADER_STAGE_VERTEX_BIT,
      0,
      sizeof(glm::mat4),
      &item.transform
    );
    vkCmdDrawIndexed(cmd, item.mesh_index_count, 1, 0, 0, 0);
  }
}

void record_prepass(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  engine::common::SharedDescriptorPool *descriptor_pool,
  engine::display::Data::Prepass *prepass,
  engine::display::Data::GPass *gpass,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::FrameInfo *frame_info,
  SessionData::Vulkan::Prepass *s_prepass,
  SessionData::Vulkan::GPass *s_gpass,
  engine::misc::RenderList *render_list
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
    render_list
  );
  vkCmdEndRenderPass(cmd);
}

void record_gpass(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  engine::common::SharedDescriptorPool *descriptor_pool,
  engine::display::Data::GPass *gpass,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::FrameInfo *frame_info,
  SessionData::Vulkan::GPass *s_gpass,
  engine::misc::RenderList *render_list
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
    render_list
  );
  vkCmdEndRenderPass(cmd);
}

void record_lpass(
  VkCommandBuffer cmd,
  SessionData::Vulkan::Core *core,
  engine::display::Data::LPass *lpass,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::FrameInfo *frame_info,
  SessionData::Vulkan::LPass *s_lpass,
  SessionData::Vulkan::FullscreenQuad *fullscreen_quad,
  VkAccelerationStructureKHR accel
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

  {
    VkWriteDescriptorSetAccelerationStructureKHR write_tlas = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &accel,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &write_tlas,
        .dstSet = lpass->descriptor_sets_frame[frame_info->inflight_index],
        .dstBinding = 5,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes),
      writes,
      0, nullptr
    );
  }
  {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &fullscreen_quad->vertex_stake.buffer, &offset);
  }
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_lpass->pipeline_layout,
    0, 1, &lpass->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );

  // @Incomplete: many lights
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    s_lpass->pipeline_layout,
    1, 1, &lpass->descriptor_sets_directional_light[frame_info->inflight_index],
    0, nullptr
  );
  vkCmdDraw(cmd, fullscreen_quad->triangle_count * 3, 1, 0, 0);
  vkCmdEndRenderPass(cmd);
}

void record_finalpass(
  VkCommandBuffer cmd,
  engine::display::Data::Finalpass *finalpass,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::LBuffer *lbuffer,
  engine::display::Data::FinalImage *final_image,
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::ZBuffer *zbuffer
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::ZBuffer *zbuffer
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::GBuffer *gbuffer
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer
) {
  ZoneScoped;
  VkImageMemoryBarrier barriers_g[] = {
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
  };

  VkImageMemoryBarrier barriers_z[] = {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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
    sizeof(barriers_g) / sizeof(*barriers_g),
    barriers_g
  );

  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    sizeof(barriers_z) / sizeof(*barriers_z),
    barriers_z
  );
}

void record_barrier_before_lpass(
  VkCommandBuffer cmd,
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::LBuffer *lbuffer
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::FinalImage *final_image
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::LBuffer *lbuffer
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
  engine::display::Data::FrameInfo *frame_info,
  engine::display::Data::FinalImage *final_image
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

struct TlasResult {
  VkAccelerationStructureKHR accel;
  lib::gfx::allocator::Buffer buffer_scratch;
  lib::gfx::allocator::Buffer buffer_accel;
  lib::gfx::allocator::Buffer buffer_instances;
  lib::gfx::allocator::Buffer buffer_instances_staging;
  lib::gfx::allocator::Buffer buffer_geometry_refs;
  lib::gfx::allocator::Buffer buffer_geometry_refs_staging;
};

struct PerInstance {
  VkDeviceAddress indices_address;
  VkDeviceAddress vertices_address;
};

void record_tlas(
  Use<SessionData::Vulkan::Core> core,
  Use<engine::misc::RenderList> render_list,
  Ref<SessionData::Vulkan::Uploader> uploader,
  VkCommandBuffer cmd,
  TlasResult *tlas_result
) {
  ZoneScoped;

  auto ext = &core->extension_pointers;

  const uint32_t instance_count = checked_integer_cast<uint32_t>(render_list->items.size());

  std::vector<VkAccelerationStructureInstanceKHR> instances(instance_count);
  for (size_t i = 0; i < instance_count; i++) {
    auto item = &render_list->items[i];
    glm::mat4 &t = item->transform;
    instances[i] = VkAccelerationStructureInstanceKHR {
      .transform = {
        t[0][0], t[1][0], t[2][0], t[3][0],
        t[0][1], t[1][1], t[2][1], t[3][1],
        t[0][2], t[1][2], t[2][2], t[3][2],
      },
      .mask = 0xFF,
      .instanceShaderBindingTableRecordOffset = 0,
      .flags = (0
        | VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR
        | VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR
      ),
      .accelerationStructureReference = item->blas_address,
    };
  }

  VkAccelerationStructureGeometryKHR geometry = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
    .geometry = {
      .instances = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
      },
    },
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
  };

	VkAccelerationStructureBuildSizesInfoKHR sizes = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };

  {
    VkAccelerationStructureBuildGeometryInfoKHR geometry_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .geometryCount = 1,
      .pGeometries = &geometry,
    };
    ext->vkGetAccelerationStructureBuildSizesKHR(
      core->device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &geometry_info,
      &instance_count,
      &sizes
    );
  }

  // @Performance: it would really be better to allocate GPU mem from separate pools.
  // Right now it will prevent a planned defragmentation mechanism in `uploader` from working.
  lib::gfx::allocator::Buffer buffer_scratch;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizes.buildScratchSize,
      .usage = (0
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_scratch = lib::gfx::allocator::create_buffer(
      &uploader->allocator_device,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_accel;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizes.accelerationStructureSize,
      .usage = (0
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_accel = lib::gfx::allocator::create_buffer(
      &uploader->allocator_device,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_instances;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = std::max(
        size_t(1),
        sizeof(VkAccelerationStructureInstanceKHR) * instance_count
      ),
      .usage = (0
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_instances = lib::gfx::allocator::create_buffer(
      &uploader->allocator_device,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_instances_staging;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = std::max(
        size_t(1),
        sizeof(VkAccelerationStructureInstanceKHR) * instance_count
      ),
      .usage = (0
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_instances_staging = lib::gfx::allocator::create_buffer(
      &uploader->allocator_host,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_geometry_refs;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = std::max(
        size_t(1),
        sizeof(PerInstance) * instance_count
      ),
      .usage = (0
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_geometry_refs = lib::gfx::allocator::create_buffer(
      &uploader->allocator_device,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_geometry_refs_staging;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = std::max(
        size_t(1),
        sizeof(PerInstance) * instance_count
      ),
      .usage = (0
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_geometry_refs_staging = lib::gfx::allocator::create_buffer(
      &uploader->allocator_host,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  {
    auto mapping = lib::gfx::allocator::get_host_mapping(
      &uploader->allocator_host,
      buffer_instances_staging.id
    );
    memcpy(
      mapping.mem,
      instances.data(),
      sizeof(VkAccelerationStructureInstanceKHR) * instances.size()
    );
  }

  {
    auto mapping = lib::gfx::allocator::get_host_mapping(
      &uploader->allocator_host,
      buffer_geometry_refs_staging.id
    );
    for (size_t i = 0; i < instances.size(); i++) {
      auto item = &render_list->items[i];
      auto destination = ((PerInstance *) mapping.mem) + i;
      destination->indices_address = item->mesh_buffer_address + item->mesh_buffer_offset_indices;
      destination->vertices_address = item->mesh_buffer_address + item->mesh_buffer_offset_vertices;
    }
  }

  if (instances.size() > 0) {
    {
      VkBufferCopy region = {
        .size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size()
      };
      vkCmdCopyBuffer(
        cmd,
        buffer_instances_staging.buffer,
        buffer_instances.buffer,
        1,
        &region
      );
    }
    {
      VkBufferCopy region = {
        .size = sizeof(PerInstance) * instances.size()
      };
      vkCmdCopyBuffer(
        cmd,
        buffer_geometry_refs_staging.buffer,
        buffer_geometry_refs.buffer,
        1,
        &region
      );
    }
  }
  
  {
    VkMemoryBarrier memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      0,
      1,
      &memory_barrier,
      0, nullptr,
      0, nullptr
    );
  }

  VkDeviceAddress instance_data_address = 0;
  {
    VkBufferDeviceAddressInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buffer_instances.buffer,
    };
    instance_data_address = vkGetBufferDeviceAddress(core->device, &info);
    assert(instance_data_address != 0);
  }

  VkDeviceAddress scratch_address = 0;
  {
    VkBufferDeviceAddressInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buffer_scratch.buffer,
    };
    scratch_address = vkGetBufferDeviceAddress(core->device, &info);
    assert(scratch_address != 0);
  }

  VkAccelerationStructureKHR accel;
  {
    VkAccelerationStructureCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = buffer_accel.buffer,
      .size = sizes.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };
    {
      auto result = ext->vkCreateAccelerationStructureKHR(
        core->device,
        &create_info,
        core->allocator,
        &accel
      );
      assert(result == VK_SUCCESS);
    }
  }

  {
    geometry.geometry.instances.data.deviceAddress = instance_data_address;
    VkAccelerationStructureBuildGeometryInfoKHR geometry_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .dstAccelerationStructure = accel,
      .geometryCount = 1,
      .pGeometries = &geometry,
      .scratchData = scratch_address,
    };
    VkAccelerationStructureBuildRangeInfoKHR range_info = {
      .primitiveCount = instance_count,
    };
    VkAccelerationStructureBuildRangeInfoKHR *range_infos[] = { &range_info };
    ext->vkCmdBuildAccelerationStructuresKHR(
      cmd,
      1,
      &geometry_info,
      range_infos
    );
  }

  {
    VkMemoryBarrier memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
      0,
      1,
      &memory_barrier,
      0, nullptr,
      0, nullptr
    );
  }

  *tlas_result = TlasResult {
    .accel = accel,
    .buffer_scratch = buffer_scratch,
    .buffer_accel = buffer_accel,
    .buffer_instances = buffer_instances,
    .buffer_instances_staging = buffer_instances_staging,
    .buffer_geometry_refs = buffer_geometry_refs,
    .buffer_geometry_refs_staging = buffer_geometry_refs_staging,
  };
}

void _tlas_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<TlasResult> result
) {
  lib::lifetime::deref(&session->lifetime, ctx->runner);

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_device,
    core->device,
    core->allocator,
    result->buffer_accel
  );

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_device,
    core->device,
    core->allocator,
    result->buffer_scratch
  );

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_device,
    core->device,
    core->allocator,
    result->buffer_instances
  );

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_host,
    core->device,
    core->allocator,
    result->buffer_instances_staging
  );

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_device,
    core->device,
    core->allocator,
    result->buffer_geometry_refs
  );

  lib::gfx::allocator::destroy_buffer(
    &session->vulkan.uploader.allocator_host,
    core->device,
    core->allocator,
    result->buffer_geometry_refs_staging
  );

  auto ext = &core->extension_pointers;
  ext->vkDestroyAccelerationStructureKHR(
    core->device,
    result->accel,
    core->allocator
  );

  delete result.ptr;
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

  auto tlas_result = new TlasResult;
  record_tlas(
    core,
    render_list,
    &session->vulkan.uploader, // horrible!
    cmd,
    tlas_result
  );

  { ZoneScopedN("tlas_cleanup_schedule");
    auto task_tlas_cleanup = defer(
      lib::task::create(
        _tlas_cleanup,
        session.ptr,
        core.ptr,
        tlas_result
      )
    );

    lib::lifetime::ref(&session->lifetime);

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_tlas_cleanup.first,
    });
    ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
      { session->inflight_gpu.signals[frame_info->inflight_index], task_tlas_cleanup.first },
    });
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
      render_list.ptr
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
      render_list.ptr
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
      core.ptr,
      lpass.ptr,
      swapchain_description.ptr,
      frame_info.ptr,
      s_lpass.ptr,
      fullscreen_quad.ptr,
      tlas_result->accel
    );
  }

  engine::rendering::intra::secondary_zbuffer::transition_to_g2(
    zbuffer2,
    frame_info,
    cmd
  );

  engine::rendering::intra::secondary_gbuffer::transition_to_g2(
    gbuffer2,
    frame_info,
    cmd
  );

  { TracyVkZone(core->tracy_context, cmd, "secondary_geometry");
    engine::rendering::pass::secondary_geometry::record(
      secondary_geometry_ddata,
      secondary_geometry_sdata,
      frame_info,
      core,
      tlas_result->buffer_geometry_refs.buffer,
      tlas_result->accel,
      cmd
    );
  }

  engine::rendering::intra::secondary_zbuffer::transition_g2_to_l2(
    zbuffer2,
    frame_info,
    cmd
  );

  engine::rendering::intra::secondary_gbuffer::transition_g2_to_l2(
    gbuffer2,
    frame_info,
    cmd
  );

  engine::rendering::intra::secondary_lbuffer::transition_to_l2(
    lbuffer2,
    frame_info,
    cmd
  );

  { TracyVkZone(core->tracy_context, cmd, "directional_light_secondary");
    engine::rendering::pass::directional_light_secondary::record(
      directional_light_secondary_ddata,
      directional_light_secondary_sdata,
      frame_info,
      swapchain_description,
      fullscreen_quad,
      descriptor_pool,
      lpass,
      core,
      tlas_result->accel,
      cmd
    );
  }

  engine::rendering::intra::secondary_lbuffer::transition_l2_to_probe_maps_update(
    lbuffer2,
    frame_info,
    cmd
  );

  engine::rendering::intra::probe_light_map::transition_into_probe_maps_update(
    probe_light_map,
    frame_info,
    cmd
  );

  { TracyVkZone(core->tracy_context, cmd, "probe_maps_update");
    engine::rendering::pass::probe_maps_update::record(
      probe_maps_update_ddata,
      probe_maps_update_sdata,
      frame_info,
      cmd
    );
  }

  engine::rendering::intra::probe_light_map::transition_probe_maps_update_into_indirect_light(
    probe_light_map,
    frame_info,
    cmd
  );

  { TracyVkZone(core->tracy_context, cmd, "indirect_light");
    engine::rendering::pass::indirect_light::record(
      cmd,
      indirect_light_ddata,
      indirect_light_sdata,
      frame_info,
      swapchain_description,
      fullscreen_quad
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
