#pragma once
#include <src/lib/task.hxx>
#include <src/global.hxx>

namespace engine::session {

void setup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<size_t> worker_count
);

} // namespace
