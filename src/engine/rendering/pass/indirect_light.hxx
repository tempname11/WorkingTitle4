#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::rendering::pass::indirect_light {

struct SData;
struct DData;

void init_sdata(
  SData *out,
  Use<engine::session::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Use<engine::session::Vulkan::Core> core
);

void init_ddata(
  DData *out,
  SData *sdata,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::Common> common,
  Use<display::Data::GBuffer> gbuffer,
  Use<display::Data::ZBuffer> zbuffer,
  Use<display::Data::LBuffer> lbuffer,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<intra::probe_depth_map::DData> probe_depth_map,
  Use<intra::probe_attention::DData> probe_attention,
  Use<display::Data::SwapchainDescription> swapchain_description
);

void deinit_ddata(
  DData *it,
  Use<engine::session::Vulkan::Core> core
);

void record(
  VkCommandBuffer cmd,
  Own<DData> ddata,
  Use<SData> sdata,
  Use<display::Data::FrameInfo> frame_info,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Use<engine::session::Vulkan::FullscreenQuad> fullscreen_quad
);

} // namespace