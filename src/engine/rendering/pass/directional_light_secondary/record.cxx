#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session.hxx>
#include "data.hxx"

namespace engine::rendering::pass::directional_light_secondary {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<SessionData::Vulkan::FullscreenQuad> fullscreen_quad,
  Ref<engine::common::SharedDescriptorPool> descriptor_pool,
  Use<engine::display::Data::LPass> lpass, // will want to remove for :ManyLights
  Use<SessionData::Vulkan::Core> core,
  VkAccelerationStructureKHR accel,
  VkCommandBuffer cmd
) {
  ZoneScoped;

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
        .dstSet = ddata->descriptor_sets_frame[frame_info->inflight_index],
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

  VkClearValue clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = sdata->render_pass,
    .framebuffer = ddata->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = {
        G2_TEXEL_SIZE.x,
        G2_TEXEL_SIZE.y,
      },
    },
    .clearValueCount = 1,
    .pClearValues = &clear_value,
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sdata->pipeline);
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = float(G2_TEXEL_SIZE.x),
    .height = float(G2_TEXEL_SIZE.y),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = {
      G2_TEXEL_SIZE.x,
      G2_TEXEL_SIZE.y,
    },
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);
  {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &fullscreen_quad->vertex_stake.buffer, &offset);
  }
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );

  size_t light_count = 1; // :ManyLights

  // @Performance: push constants for lights would be better here.
  std::vector<VkDescriptorSet> descriptor_sets(light_count);
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts(light_count);
  for (size_t i = 0; i < light_count; i++) {
    descriptor_set_layouts[i] = sdata->descriptor_set_layout_light;
  }
  { ZoneScoped("descriptor_sets");
    {
      std::scoped_lock lock(descriptor_pool->mutex);
      VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool->pool,
        .descriptorSetCount = checked_integer_cast<uint32_t>(light_count),
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

  for (size_t i = 0; i < light_count; i++) {
    auto descriptor_set = descriptor_sets[i];
    VkDescriptorBufferInfo light_info = {
      .buffer = lpass->stakes.ubo_directional_light[frame_info->inflight_index].buffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &light_info,
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
      sdata->pipeline_layout,
      1, 1, &descriptor_set,
      0, nullptr
    );

    vkCmdDraw(cmd, fullscreen_quad->triangle_count * 3, 1, 0, 0);
  }
  vkCmdEndRenderPass(cmd);
}

} // namespace
