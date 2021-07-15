#include <thread>
#include <vector>
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/task/session.hxx>

// #define ENGINE_DEBUG_TASK_THREADS 1

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
  #if ENGINE_DEBUG_TASK_THREADS == 1
    | QUEUE_ACCESS_FLAGS_WORKER_THREAD
  #endif
);

int main() {
  #ifdef ENGINE_DEBUG_TASK_THREADS
    unsigned int num_threads = ENGINE_DEBUG_TASK_THREADS;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  #if ENGINE_DEBUG_TASK_THREADS == 1
    size_t worker_count = 1;
    std::vector<task::QueueAccessFlags> worker_access_flags;
  #else
    size_t worker_count = num_threads - 1;
    std::vector<lib::task::QueueAccessFlags> worker_access_flags(worker_count, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
  #endif
  auto runner = lib::task::create_runner(QUEUE_COUNT);
  lib::task::inject(runner, {
    lib::task::create(session, &worker_count),
  });
  lib::task::run(runner, std::move(worker_access_flags), QUEUE_ACCESS_FLAGS_MAIN_THREAD);
  lib::task::discard_runner(runner);
  #ifdef TRACY_NO_EXIT
    printf("Waiting for profiler...\n");
    fflush(stdout);
  #endif
  return 0;
}

#ifdef WINDOWS
  int WinMain() {
    return main();
  }
#endif