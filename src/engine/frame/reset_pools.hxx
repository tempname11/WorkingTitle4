#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void reset_pools(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::CommandPools> command_pools,
  Own<engine::display::Data::DescriptorPools> descriptor_pools,
  Ref<engine::display::Data::FrameInfo> frame_info
);

} // namespace
