#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::step::probe_appoint {

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
  Own<display::Data::Helpers> helpers,
  Use<datum::probe_attention::DData> probe_attention,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_workset::SData> probe_workset,
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
  Ref<engine::datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::session::Vulkan::Core> core,
  VkCommandBuffer cmd
);

} // namespace
