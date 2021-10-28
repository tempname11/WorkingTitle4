#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/misc.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::step::probe_measure {

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
  engine::display::Data::LPass::Stakes* lpass_stakes,
  Use<datum::probe_radiance::DData> lbuffer,
  Use<datum::probe_irradiance::DData> probe_irradiance,
  Use<datum::probe_attention::DData> probe_attention,
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
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::session::Vulkan::Core> core,
  Ref<engine::common::SharedDescriptorPool> descriptor_pool,
  Ref<engine::datum::probe_workset::SData> probe_workset,
  VkBuffer geometry_refs,
  Use<engine::misc::RenderList> render_list,
  VkAccelerationStructureKHR accel,
  VkCommandBuffer cmd
);

} // namespace
