#pragma once
#include <src/engine/session/data.hxx>

// @Note: maybe we should go the `defer` route here,
// and make this a function returning a pair of tasks.
void after_inflight(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<lib::Task> task
);
