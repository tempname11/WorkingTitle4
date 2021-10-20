#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void reset_pools(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Ref<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::DescriptorPools> descriptor_pools,
  Use<engine::display::Data::FrameInfo> frame_info
);

} // namespace
