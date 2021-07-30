#pragma once
#include <vector>
#include "task.hxx"

void defer(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::None<task::Task> task
);

void defer_many(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<std::vector<task::Task *>> tasks
);