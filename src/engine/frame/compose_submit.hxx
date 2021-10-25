#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void compose_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Own<engine::display::Data::Presentation> presentation,
  Ref<VkSemaphore> imgui_finished_semaphore,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ComposeData> data
);

} // namespace
