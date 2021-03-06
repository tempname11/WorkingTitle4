#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::frame {

void setup_gpu_signal(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<VkSemaphore> frame_finished_semaphore,
  Ref<engine::display::Data::FrameInfo> frame_info
);

} // namespace
