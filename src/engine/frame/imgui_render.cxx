#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <src/engine/session/data/vulkan.hxx>
#include "imgui_render.hxx"

namespace engine::frame {

void imgui_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::VulkanData::Core> core,
  Own<engine::session::Data::ImguiContext> imgui_context,
  Own<engine::display::Data::ImguiBackend> imgui_backend,
  Own<engine::session::Data::GLFW> glfw, /* hacky way to prevent KeyMods bug. */
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ImguiData> data
) {
  ZoneScoped;
  ImGui::Render();
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
  auto drawData = ImGui::GetDrawData();
  { // begin render pass
    auto begin_info = VkRenderPassBeginInfo {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = imgui_backend->render_pass,
      .framebuffer = imgui_backend->framebuffers[frame_info->inflight_index],
      .renderArea = {
        .extent = swapchain_description->image_extent,
      },
      .clearValueCount = 0,
      .pClearValues = nullptr,
    };
    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
  }
  ImGui_ImplVulkan_RenderDrawData(
    drawData,
    cmd
  );
  vkCmdEndRenderPass(cmd);
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  pool1->used_buffers.push_back(cmd);
  command_pool_2_return(pool2, pool1);
  data->cmd = cmd;
}

} // namespace
