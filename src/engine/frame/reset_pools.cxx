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
    auto pool2 = &(*command_pools)[frame_info->inflight_index];
    // std::scoped_lock lock(pool2->mutex); we have Own
    for (auto pool1 : pool2->available) {
      {
        auto result = vkResetCommandPool(
          core->device,
          pool1->pool,
          0
        );
        assert(result == VK_SUCCESS);
      }

      if (pool1->used_buffers.size() > 0) {
        vkFreeCommandBuffers(
          core->device,
          pool1->pool,
          pool1->used_buffers.size(),
          pool1->used_buffers.data()
        );
        pool1->used_buffers.clear();
      }
    }
  }
  { ZoneScopedN("descriptor");
    auto pool = &(*descriptor_pools)[frame_info->inflight_index];
    // std::scoped_lock lock(pool->mutex); we have Own
    auto result = vkResetDescriptorPool(
      core->device,
      pool->pool,
      0
    );
    assert(result == VK_SUCCESS);
  }
}

} // namespace
