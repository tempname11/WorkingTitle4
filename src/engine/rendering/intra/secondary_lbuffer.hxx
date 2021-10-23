#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::rendering::intra::secondary_lbuffer {

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

void transition_into_probe_measure(
  Use<DData> it,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_from_probe_measure_into_collect(
  Use<DData> it,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace