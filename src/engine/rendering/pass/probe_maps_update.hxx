#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>

namespace engine::rendering::pass::probe_maps_update {

struct SData;
struct DData;

void init_sdata(
  SData *out,
  Use<SessionData::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Use<SessionData::Vulkan::Core> core
);

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<engine::display::Data::Common> common,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<SessionData::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Use<SessionData::Vulkan::Core> core
);

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace
