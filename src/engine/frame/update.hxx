#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void update(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<misc::UpdateData> update,
  Ref<display::Data::FrameInfo> frame_info,
  Use<display::Data::Readback> readback_data,
  Own<session::Data::State> session_state,
  Own<system::ode::Impl> ode
);

} // namespace
