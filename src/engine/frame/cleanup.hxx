#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::misc::FrameData> frame_data
);

} // namespace
