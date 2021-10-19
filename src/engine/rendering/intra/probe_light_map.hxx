#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::rendering::intra::probe_light_map {

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

void transition_from_update_into_indirect_light(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_into_update(
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

void transition_previous_into_update(
  Use<DData> it,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

} // namespace