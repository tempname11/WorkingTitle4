#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void compose_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display,
  Ref<engine::session::Vulkan::Core> core,
  Use<engine::display::Data::Presentation> presentation,
  Use<engine::display::Data::PresentationFailureState> presentation_failure_state,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::FinalImage> final_image,
  Own<engine::misc::ComposeData> data
);

} // namespace
