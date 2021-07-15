#pragma once
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void rendering_has_finished( \
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx, \
  usage::Full<task::Task> rendering_yarn_end \
)

TASK_DECL;
