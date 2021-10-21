#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::rendering::intra::probe_attention {

struct DData;

void init_ddata(
  DData *out,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Use<engine::session::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Use<engine::session::Vulkan::Core> core
);

void clear_and_transition_into_lpass(
  DData *it,
  Use<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_previous_from_write_to_read(
  DData *it,
  Use<display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

} // namespace