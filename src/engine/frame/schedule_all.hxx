#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void schedule_all(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<lib::task::Task> rendering_yarn_end,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display
);

} // namespace
