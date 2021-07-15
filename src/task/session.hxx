#pragma once
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void session( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::None<size_t> worker_count \
)

TASK_DECL;
