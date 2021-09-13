#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "indirect_light/data.hxx"

namespace engine::rendering::pass::indirect_light {

void init_sdata(
  SData *out,
  Use<SessionData::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Use<SessionData::Vulkan::Core> core
);

void init_rdata(
  RData *out,
  SData *sdata,
  Use<SessionData::Vulkan::Core> core,
  Own<engine::display::Data::Common> common,
  Use<engine::display::Data::LBuffer> lbuffer,
  Use<engine::display::Data::SwapchainDescription> swapchain_description
);

void deinit_rdata(
  RData *it,
  Use<SessionData::Vulkan::Core> core
);

void record(
  VkCommandBuffer cmd,
  Own<RData> rdata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<SessionData::Vulkan::FullscreenQuad> fullscreen_quad
);

} // namespace