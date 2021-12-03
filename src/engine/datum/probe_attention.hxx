#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::datum::probe_attention {

struct DData;

void init_ddata(
  DData *out,
  Ref<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Ref<engine::session::VulkanData::Core> core
);

void deinit_ddata(
  DData *it,
  Ref<engine::session::VulkanData::Core> core
);

void clear_and_transition_into_lpass(
  DData *it,
  Ref<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void transition_previous_from_write_to_read(
  DData *it,
  Ref<display::Data::FrameInfo> frame_info,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  VkCommandBuffer cmd
);

} // namespace