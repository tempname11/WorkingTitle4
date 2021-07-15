#pragma once
#include <src/engine/session.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void session_cleanup( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Full<SessionData> session \
)

TASK_DECL;
