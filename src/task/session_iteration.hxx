#pragma once
#include <src/engine/session/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void session_iteration( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Some<engine::session::Data> session \
)

TASK_DECL;
