#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data/vulkan.hxx>
#include <src/engine/display/data.hxx>

namespace engine::step::probe_appoint {

struct SData;
struct DData;

void init_sdata(
  SData *out,
  Ref<engine::session::VulkanData::Core> core
);

void deinit_sdata(
  SData *it,
  Ref<engine::session::VulkanData::Core> core
);

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<display::Data::Helpers> helpers,
  Use<datum::probe_attention::DData> probe_attention,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_offsets::SData> probe_offsets,
  Use<datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::session::VulkanData::Core> core
);

void deinit_ddata(
  DData *it,
  Ref<engine::session::VulkanData::Core> core
);

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Ref<engine::datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::session::VulkanData::Core> core,
  VkCommandBuffer cmd
);

} // namespace
