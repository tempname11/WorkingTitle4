#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_prepare_uniforms( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Use<SessionData::Vulkan::Core> core, \
  Use<RenderingData::SwapchainDescription> swapchain_description, \
  Use<RenderingData::FrameInfo> frame_info, \
  Use<SessionData::State> session_state, \
  Own<RenderingData::Common> common, \
  Own<RenderingData::GPass> gpass, \
  Own<RenderingData::LPass> lpass \
)

TASK_DECL;
