#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_prepare_uniforms( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Use<SessionData::Vulkan::Core> core, \
  Use<engine::display::Data::SwapchainDescription> swapchain_description, \
  Use<engine::display::Data::FrameInfo> frame_info, \
  Use<SessionData::State> session_state, \
  Own<engine::display::Data::Common> common, \
  Own<engine::display::Data::GPass> gpass, \
  Own<engine::display::Data::LPass> lpass \
)

TASK_DECL;
