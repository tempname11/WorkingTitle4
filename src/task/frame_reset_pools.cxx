#include "task.hxx"

void frame_reset_pools(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::CommandPools> command_pools,
  usage::Full<RenderingData::DescriptorPools> descriptor_pools,
  usage::Some<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  {
    auto &pool2 = (*command_pools)[frame_info->inflight_index];
    // Skip the lock, we have usage::Full!
    // std::scoped_lock lock(pool2.mutex);
    for (auto pool : pool2.pools) {
      vkResetCommandPool(
        core->device,
        pool,
        0
      );
    }
  }
  {
    auto &pool = (*descriptor_pools)[frame_info->inflight_index];
    // Skip the lock, we have usage::Full!
    // std::scoped_lock lock(pool.mutex);
    vkResetDescriptorPool(
      core->device,
      pool.pool,
      0
    );
  }
}
