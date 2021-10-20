#include "reset_pools.hxx"

namespace engine::frame {

void reset_pools(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::CommandPools> command_pools,
  Own<engine::display::Data::DescriptorPools> descriptor_pools,
  Use<engine::display::Data::FrameInfo> frame_info
) {
  ZoneScoped;
  { ZoneScopedN("command");
    auto &pool2 = (*command_pools)[frame_info->inflight_index];
    // Skip the lock, we have `Own`!
    // std::scoped_lock lock(pool2.mutex);
    for (auto pool : pool2.pools) {
      auto result = vkResetCommandPool(
        core->device,
        pool,
        0
      );
      assert(result == VK_SUCCESS);
    }
  }
  { ZoneScopedN("descriptor");
    auto &pool = (*descriptor_pools)[frame_info->inflight_index];
    // Skip the lock, we have `Own`!
    // std::scoped_lock lock(pool.mutex);
    auto result = vkResetDescriptorPool(
      core->device,
      pool.pool,
      0
    );
    assert(result == VK_SUCCESS);
  }
}

} // namespace
