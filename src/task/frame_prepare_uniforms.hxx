#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_prepare_uniforms( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Some<RenderingData::SwapchainDescription> swapchain_description, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Some<SessionData::State> session_state, \
  usage::Full<RenderingData::Common> common, \
  usage::Full<RenderingData::GPass> gpass, \
  usage::Full<RenderingData::LPass> lpass \
)

TASK_DECL;
