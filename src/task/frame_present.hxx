#pragma once
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_present( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<RenderingData::Presentation> presentation, \
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<VkQueue> queue_present \
)

TASK_DECL;
