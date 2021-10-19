#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void acquire(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::Presentation> presentation,
  Use<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Use<engine::display::Data::FrameInfo> frame_info
);

} // namespace
