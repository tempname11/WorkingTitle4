#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::step::indirect_light {

struct SData;
struct DData;

void init_sdata(
  SData *out,
  Ref<engine::session::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Ref<engine::session::Vulkan::Core> core
);

void init_ddata(
  DData *out,
  SData *sdata,
  Ref<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::Helpers> helpers,
  Use<display::Data::GBuffer> gbuffer,
  Use<display::Data::ZBuffer> zbuffer,
  Use<display::Data::LBuffer> lbuffer,
  Use<datum::probe_irradiance::DData> probe_irradiance,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_offsets::SData> probe_offsets,
  Use<datum::probe_attention::DData> probe_attention,
  Ref<display::Data::SwapchainDescription> swapchain_description
);

void deinit_ddata(
  DData *it,
  Ref<engine::session::Vulkan::Core> core
);

void record(
  VkCommandBuffer cmd,
  Own<DData> ddata,
  Use<SData> sdata,
  Ref<display::Data::FrameInfo> frame_info,
  Ref<display::Data::SwapchainDescription> swapchain_description,
  Use<engine::session::Vulkan::FullscreenQuad> fullscreen_quad
);

} // namespace