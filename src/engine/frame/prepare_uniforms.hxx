#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void prepare_uniforms(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Core> core,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Use<engine::session::Data::State> session_state,
  Own<engine::display::Data::Common> common,
  Own<engine::display::Data::GPass> gpass,
  Own<engine::display::Data::LPass> lpass
);

} // namespace
