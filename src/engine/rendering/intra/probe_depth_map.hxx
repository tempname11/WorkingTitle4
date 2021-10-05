#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session.hxx>

namespace engine::rendering::intra::probe_depth_map {

struct DData;

void init_ddata(
  DData *out,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Use<SessionData::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Use<SessionData::Vulkan::Core> core
);

void transition_probe_maps_update_into_indirect_light(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_into_probe_maps_update(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

void transition_previous_into_l2(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

void transition_previous_into_probe_maps_update(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

} // namespace