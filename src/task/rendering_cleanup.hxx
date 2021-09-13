#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void rendering_cleanup( \
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx, \
  usage::Full<task::Task> session_iteration_yarn_end, \
  usage::Some<SessionData> session, \
  usage::Full<engine::display::Data> data \
)

TASK_DECL;
