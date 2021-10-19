#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::rendering::pass::probe_depth_update {

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
  Use<SData> sdata,
  Own<engine::display::Data::Common> common,
  Use<intra::secondary_zbuffer::DData> secondary_zbuffer,
  Use<intra::probe_depth_map::DData> probe_depth_map,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::session::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Use<engine::session::Vulkan::Core> core
);

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace
