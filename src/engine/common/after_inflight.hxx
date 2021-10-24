#pragma once
#include <src/engine/session/data.hxx>

namespace engine::common {

// Like `defer`, this should be a function returning a pair of tasks.
void after_inflight(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<lib::Task> task
);

} // namespace
