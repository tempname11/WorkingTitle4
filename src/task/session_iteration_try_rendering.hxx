#pragma once
#include <src/engine/session/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void session_iteration_try_rendering( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<task::Task> session_iteration_yarn_end, \
  usage::Full<engine::session::Data> session \
)

TASK_DECL;
