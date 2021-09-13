#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_update( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<engine::misc::UpdateData> update, \
  usage::Some<engine::display::Data::FrameInfo> frame_info, \
  usage::Full<SessionData::State> session_state \
)

TASK_DECL;
