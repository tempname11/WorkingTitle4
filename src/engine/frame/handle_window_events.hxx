#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void handle_window_events(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data::GLFW> glfw,
  Own<engine::session::Data::State> session_state,
  Own<engine::misc::UpdateData> update
);

} // namespace
