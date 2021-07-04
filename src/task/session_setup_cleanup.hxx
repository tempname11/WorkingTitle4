#include "task.hxx"

#define TASK_DECL void session_setup_cleanup(\
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY>* ctx,\
  usage::Full<SessionSetupData> data,\
  usage::Some<SessionData::Vulkan::Core> core\
)

TASK_DECL;
