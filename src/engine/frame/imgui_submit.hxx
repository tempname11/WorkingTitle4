#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void imgui_submit(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Ref<VkSemaphore> graphics_finished_semaphore,
  Ref<VkSemaphore> imgui_finished_semaphore,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ImguiData> data
);

} // namespace