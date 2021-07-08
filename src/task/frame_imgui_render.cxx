#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include "task.hxx"

void frame_imgui_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::ImguiContext> imgui_context,
  usage::Full<RenderingData::ImguiBackend> imgui_backend,
  usage::Full<SessionData::GLFW> glfw, // hacky way to prevent KeyMods bug.
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
) {
  ZoneScoped;
  ImGui::Render();
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
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}
