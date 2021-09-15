#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session.hxx>

namespace engine::rendering::intra::secondary_lbuffer {

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

void transition_all_secondary_light_into_probe_maps_update(
  Use<DData> it,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace