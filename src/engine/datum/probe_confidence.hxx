#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>

namespace engine::datum::probe_confidence {

struct SData;

void init_sdata(
  SData *out,
  Ref<lib::gfx::Allocator> allocator_device,
  Ref<engine::session::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Ref<engine::session::Vulkan::Core> core
);

void barrier_into_appoint(
  SData *it,
  Ref<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void barrier_from_appoint_into_measure(
  SData *it,
  Ref<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void barrier_from_measure_into_collect(
  SData *it,
  Ref<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

void barrier_from_collect_into_indirect_light(
  SData *it,
  Ref<display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace