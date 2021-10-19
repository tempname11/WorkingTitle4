#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::indirect_light {

void record(
  VkCommandBuffer cmd,
  Own<DData> ddata, // why Own?
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::session::Vulkan::FullscreenQuad> fullscreen_quad
) {
  ZoneScoped;

  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = sdata->render_pass,
    .framebuffer = ddata->framebuffers[frame_info->inflight_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
    },
  };
  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sdata->pipeline);
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
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );
  {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &fullscreen_quad->vertex_stake.buffer, &offset);
  }
  vkCmdDraw(cmd, fullscreen_quad->triangle_count * 3, 1, 0, 0);
  vkCmdEndRenderPass(cmd);
}

} // namespace
