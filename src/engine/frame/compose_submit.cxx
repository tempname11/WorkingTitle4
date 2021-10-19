#include "compose_submit.hxx"

namespace engine::frame {

void compose_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Own<engine::display::Data::Presentation> presentation,
  Use<VkSemaphore> imgui_finished_semaphore,
  Use<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ComposeData> data
) {
  ZoneScoped;
  VkSemaphore wait_semaphores[] = {
    *imgui_finished_semaphore,
    presentation->image_acquired[frame_info->inflight_index]
  };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
  };
  uint64_t wait_values[] = {
    frame_info->timeline_semaphore_value,
    0,
  };
  VkSemaphore signal_semaphores[] = {
    presentation->image_rendered[frame_info->inflight_index],
  };
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .waitSemaphoreValueCount = sizeof(wait_values) / sizeof(*wait_values),
    .pWaitSemaphoreValues = wait_values,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .waitSemaphoreCount = sizeof(wait_semaphores) / sizeof(*wait_semaphores),
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(*signal_semaphores),
    .pSignalSemaphores = signal_semaphores,
  };
  if (data->cmd == VK_NULL_HANDLE) {
    submit_info.waitSemaphoreCount -= 1; // do not wait for image
    timeline_info.waitSemaphoreValueCount -= 1;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = nullptr;
  }
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
}

} // namespace
