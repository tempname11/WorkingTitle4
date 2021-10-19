#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::display {

void cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::Task> session_iteration_yarn_end,
  Use<engine::session::Data> session,
  Own<engine::display::Data> data
);

} // namespace
