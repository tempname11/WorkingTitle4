#include "session_setup_cleanup.hxx"

TASK_DECL {
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
