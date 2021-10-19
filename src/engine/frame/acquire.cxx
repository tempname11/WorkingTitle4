#include "acquire.hxx"

namespace engine::frame {

void acquire(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::Presentation> presentation,
  Use<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Use<engine::display::Data::FrameInfo> frame_info
) {
  ZoneScoped;
  uint32_t image_index;
  auto result = vkAcquireNextImageKHR(
    core->device,
    presentation->swapchain,
    0,
    presentation->image_acquired[frame_info->inflight_index],
    VK_NULL_HANDLE,
    &presentation->latest_image_index
  );
  if (result == VK_SUBOPTIMAL_KHR) {
    // not sure what to do with this.
    // log and treat as success.
    LOG("vkAcquireNextImageKHR: VK_SUBOPTIMAL_KHR");
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
