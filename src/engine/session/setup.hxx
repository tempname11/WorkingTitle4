#pragma once
#include <src/lib/task.hxx>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>

namespace engine::session {

void setup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data *> out
);

} // namespace
