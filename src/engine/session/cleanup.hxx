#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>

namespace engine::session {

void cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<SessionData> session
);

} // namespace
