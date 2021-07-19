#include "frame_compose_submit.hxx"

TASK_DECL {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      // we still need to signal that gpu work is over.
      // so do a dummy submit that has no commands,
      // just "links" the semaphores
      auto timeline_info = VkTimelineSemaphoreSubmitInfo {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &frame_info->timeline_semaphore_value,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
      };
      VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = imgui_finished_semaphore.ptr,
        .pWaitDstStageMask = &wait_stage,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = frame_rendered_semaphore.ptr,
      };
      auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
      return;
    }
  }
  uint64_t zero = 0;
  VkSemaphore wait_semaphores[] = {
    *imgui_finished_semaphore,
    presentation->image_acquired[frame_info->inflight_index]
  };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
  };
  uint64_t wait_values[] = {
    frame_info->timeline_semaphore_value,
    zero,
  };
  VkSemaphore signal_semaphores[] = {
    *frame_rendered_semaphore,
    presentation->image_rendered[frame_info->inflight_index],
  };
  uint64_t signal_values[] = {
    frame_info->timeline_semaphore_value,
    zero,
  };
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .waitSemaphoreValueCount = sizeof(wait_values) / sizeof(*wait_values),
    .pWaitSemaphoreValues = wait_values,
    .signalSemaphoreValueCount = sizeof(signal_values) / sizeof(*signal_values),
    .pSignalSemaphoreValues = signal_values,
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
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
}
