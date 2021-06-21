#include "task.hxx"

void rendering_frame_imgui_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Some<VkSemaphore> example_finished_semaphore,
  usage::Some<VkSemaphore> imgui_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
) {
  ZoneScoped;
  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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
    .pWaitSemaphores = example_finished_semaphore.ptr,
    .pWaitDstStageMask = &wait_stage,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = imgui_finished_semaphore.ptr,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
  delete data.ptr;
}