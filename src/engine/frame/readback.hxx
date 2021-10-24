#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::frame {

void readback(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Own<engine::display::Data::Presentation> presentation,
  Use<VkSemaphore> frame_finished_semaphore,
  Use<engine::display::Data::LBuffer> lbuffer,
  Use<engine::display::Data::Readback> readback_data,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::display::Data::FrameInfo> frame_info
);

} // namespace
