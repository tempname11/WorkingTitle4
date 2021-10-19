#include "graphics_submit.hxx"

namespace engine::frame {

void graphics_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Use<VkSemaphore> example_finished_semaphore,
  Use<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::GraphicsData> data
) {
  ZoneScoped;
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = example_finished_semaphore.ptr,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
}

} // namespace
