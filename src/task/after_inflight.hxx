#pragma once
#include <src/engine/session.hxx>
#include "task.hxx"

// @Note: maybe we should go the `defer` route here,
// and make this a function returning a pair of tasks.
void after_inflight(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<SessionData> session,
  Ref<task::Task> task
);
