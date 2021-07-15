#pragma once
#include <src/engine/session.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_handle_window_events( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Full<SessionData::GLFW> glfw, \
  usage::Full<SessionData::State> session_state, \
  usage::Full<UpdateData> update \
)

TASK_DECL;
