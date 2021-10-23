#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::datum::probe_irradiance {

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

void transition_from_probe_collect_into_indirect_light(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_into_probe_collect(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

void transition_previous_into_probe_measure(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

void transition_previous_from_probe_measure_into_collect(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

} // namespace