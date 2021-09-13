#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_acquire( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Full<engine::display::Data::Presentation> presentation, \
  usage::Some<engine::display::Data::PresentationFailureState> presentation_failure_state, \
  usage::Some<engine::display::Data::FrameInfo> frame_info \
)

TASK_DECL;
