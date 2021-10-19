#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_handle_window_events( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  Own<engine::session::Data::GLFW> glfw, \
  Own<engine::session::Data::State> session_state, \
  Own<engine::misc::UpdateData> update \
)

TASK_DECL;
