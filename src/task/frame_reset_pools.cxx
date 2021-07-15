#include "frame_reset_pools.hxx"

TASK_DECL {
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
