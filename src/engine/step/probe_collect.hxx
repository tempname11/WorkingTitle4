#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::step::probe_collect {

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
  Use<SData> sdata,
  Own<engine::display::Data::Common> common,
  Use<datum::probe_radiance::DData> probe_radiance,
  Use<datum::probe_irradiance::DData> probe_irradiance,
  Use<datum::probe_attention::DData> probe_attention,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::session::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Ref<engine::session::Vulkan::Core> core
);

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Ref<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace
