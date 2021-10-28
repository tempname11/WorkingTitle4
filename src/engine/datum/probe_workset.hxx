#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>

namespace engine::datum::probe_workset {

struct SData;

void init_sdata(
  SData *out,
  Ref<engine::session::Vulkan::Core> core,
  Ref<lib::gfx::Allocator> allocator_device
);

void deinit_sdata(
  SData *it,
  Ref<engine::session::Vulkan::Core> core
);

void barrier_from_write_to_read(
  SData *it,
  Ref<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
);

} // namespace