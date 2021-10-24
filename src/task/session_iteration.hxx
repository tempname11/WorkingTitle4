#pragma once
#include <src/engine/session/data.hxx>

void session_iteration(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<engine::session::Data> session
);
