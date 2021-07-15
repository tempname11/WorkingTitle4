#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void rendering_frame( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<task::Task> rendering_yarn_end, \
  usage::None<SessionData> session, \
  usage::None<RenderingData> data, \
  usage::Some<SessionData::GLFW> glfw, \
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state, \
  usage::Full<RenderingData::FrameInfo> latest_frame, \
  usage::Some<RenderingData::SwapchainDescription> swapchain_description, \
  usage::Some<RenderingData::InflightGPU> inflight_gpu \
)

TASK_DECL;
