#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void graphics_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Ref<VkSemaphore> graphics_finished_semaphore,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::GraphicsData> data
);

} // namespace
