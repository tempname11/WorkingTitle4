#pragma once
#include <src/engine/session.hxx>
#include "task.hxx"

void defer(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<task::Task> task
);

void defer_many(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<std::vector<task::Task *>> tasks
);