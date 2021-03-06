#include "present.hxx"

namespace engine::frame {

void present(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<engine::display::Data::Presentation> presentation,
  Ref<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<VkQueue> queue_present
) {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      return;
    }
  }
  VkPresentInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &presentation->image_rendered[frame_info->inflight_index],
    .swapchainCount = 1,
    .pSwapchains = &presentation->swapchain,
    .pImageIndices = &presentation->latest_image_index,
  };
  auto result = vkQueuePresentKHR(*queue_present, &info);
  if (result == VK_SUBOPTIMAL_KHR) {
    // not sure what to do with this.
    // log and treat as success.
    LOG("vkQueuePresentKHR: VK_SUBOPTIMAL_KHR");
  } else if (false
    || result == VK_ERROR_OUT_OF_DATE_KHR
    || result == VK_ERROR_SURFACE_LOST_KHR
    || result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
  ) {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_failure_state->failure = true;
  } else {
    assert(result == VK_SUCCESS);
  }
}

} // namespace
