#pragma once
#include <src/engine/session.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void session_iteration( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Full<task::Task> session_yarn_end, \
  usage::Some<SessionData> data \
)

TASK_DECL;
