#include "imgui_submit.hxx"

namespace engine::frame {

void imgui_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Use<VkSemaphore> graphics_finished_semaphore,
  Use<VkSemaphore> imgui_finished_semaphore,
  Use<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ImguiData> data
) {
  ZoneScoped;
  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .waitSemaphoreValueCount = 1,
    .pWaitSemaphoreValues = &frame_info->timeline_semaphore_value,
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = graphics_finished_semaphore.ptr,
    .pWaitDstStageMask = &wait_stage,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = imgui_finished_semaphore.ptr,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
}

} // namespace