#pragma once
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_cleanup( \
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx, \
  usage::Full<RenderingData::FrameInfo> frame_info, \
  usage::Full<engine::misc::FrameData> frame_data \
)

TASK_DECL;
