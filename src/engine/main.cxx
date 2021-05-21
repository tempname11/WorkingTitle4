#include <src/lib/task.hxx>
#include <src/global.hxx>

void task_engine_cleanup(lib::task::Context *ctx) {
  ZoneScoped;
}

void task_engine_idle_loop(lib::task::Context *ctx) {
  ZoneScoped;
  lib::task::inject(ctx->runner, {
    lib::task::describe(
      QUEUE_INDEX_MAIN_THREAD_ONLY,
      task_engine_idle_loop
    ),
  });
}

void task_engine(lib::task::Context *ctx) {
  ZoneScoped;
  auto stop_signal = lib::task::create_signal();
  ctx->subtasks = {
    lib::task::describe(
      QUEUE_INDEX_MAIN_THREAD_ONLY,
      task_engine_idle_loop
    ),
  };
  ctx->signals = { stop_signal };
}

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
);

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

int main() {
  auto runner = lib::task::create_runner(QUEUE_COUNT);
  lib::task::inject(runner, {
    lib::task::describe(
      QUEUE_INDEX_MAIN_THREAD_ONLY,
      task_engine
    ),
  });
  #ifndef NDEBUG
    auto num_threads = 4u;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  std::vector worker_access_flags(num_threads - 1, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
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