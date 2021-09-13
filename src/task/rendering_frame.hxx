#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void rendering_frame( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<task::Task> rendering_yarn_end, \
  usage::None<SessionData> session, \
  usage::None<engine::display::Data> data, \
  usage::Some<SessionData::GLFW> glfw, \
  usage::Some<engine::display::Data::PresentationFailureState> presentation_failure_state, \
  usage::Full<engine::display::Data::FrameInfo> latest_frame, \
  usage::Some<engine::display::Data::SwapchainDescription> swapchain_description \
)

TASK_DECL;
