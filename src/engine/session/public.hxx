#pragma once
#include <src/global.hxx>

namespace engine::session {

struct Data;

void setup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<Data *> out
);

void cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data> session
);

} // namespace
