#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void imgui_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Core> core,
  Own<engine::session::Data::ImguiContext> imgui_context,
  Own<engine::display::Data::ImguiBackend> imgui_backend,
  Own<engine::session::Data::GLFW> glfw,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Own<engine::misc::ImguiData> data
);

} // namespace
