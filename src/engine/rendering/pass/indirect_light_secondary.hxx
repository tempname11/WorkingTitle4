#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>

namespace engine::rendering::pass::indirect_light_secondary {

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
  Own<display::Data::Common> common,
  Use<intra::secondary_zbuffer::DData> zbuffer2,
  Use<intra::secondary_gbuffer::DData> gbuffer2,
  Use<intra::secondary_lbuffer::DData> lbuffer2,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<intra::probe_depth_map::DData> probe_depth_map,
  Use<display::Data::SwapchainDescription> swapchain_description,
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
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<SessionData::Vulkan::FullscreenQuad> fullscreen_quad,
  VkCommandBuffer cmd
);

} // namespace