#include "session_setup_cleanup.hxx"

void session_setup_cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::SetupData> data,
  Use<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  vkDestroyCommandPool(
   core->device,
   data->command_pool,
   core->allocator
  );

  vkDestroySemaphore(
   core->device,
   data->semaphore_finished,
   core->allocator
  );

  delete data.ptr;
}
