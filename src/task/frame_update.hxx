#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_update( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<UpdateData> update, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<SessionData::State> session_state \
)

TASK_DECL;
