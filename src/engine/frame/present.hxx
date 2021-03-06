#pragma once
#include <src/engine/display/data.hxx>

namespace engine::frame {

void present(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<engine::display::Data::Presentation> presentation,
  Ref<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<VkQueue> queue_present
);

} // namespace
