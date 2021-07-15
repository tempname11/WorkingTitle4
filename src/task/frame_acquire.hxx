#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_acquire( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Full<RenderingData::Presentation> presentation, \
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state, \
  usage::Some<RenderingData::FrameInfo> frame_info \
)

TASK_DECL;
