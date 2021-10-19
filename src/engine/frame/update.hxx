#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void update(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::misc::UpdateData> update,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::Readback> readback_data,
  Own<engine::session::Data::State> session_state
);

} // namespace
