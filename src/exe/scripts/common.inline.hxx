#pragma once
#include <semaphore>
#include <src/lib/task.hxx>

struct Waitable {
  bool should_delete;
  std::binary_semaphore *semaphore;

  Waitable(std::binary_semaphore *semaphore_) {
    semaphore = semaphore_;
    should_delete = true;
  }

  Waitable(Waitable &other) {
    semaphore = other.semaphore;
    should_delete = false;
    semaphore->release();
  }

  Waitable(Waitable &&other) {
    semaphore = other.semaphore;
    should_delete = other.should_delete;
    other.should_delete = false;
  }

  ~Waitable() {
    semaphore->acquire();
    if (should_delete) {
      delete semaphore;
    }
  }
};

void _release_semaphore(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<std::binary_semaphore> semaphore
) {
  semaphore->release();
}

struct Ctrl {
  lib::task::ContextBase *ctx;
  std::vector<Waitable> forgotten;

  Waitable wait_for_signal(lib::Task *signal) {
    auto semaphore = new std::binary_semaphore(0);
    auto after = lib::task::create(_release_semaphore, semaphore);
    lib::task::inject(ctx->runner, {}, {
      .new_dependencies = {
        { signal, after },
      },
    });
    return Waitable(semaphore);
  }

  Waitable task(lib::Task *task) {
    auto semaphore = new std::binary_semaphore(0);
    auto after = lib::task::create(_release_semaphore, semaphore);
    lib::task::inject(ctx->runner, { task }, {
      .new_dependencies = {
        { task, after },
      },
    });
    return Waitable(semaphore);
  }

  template <typename... Args>
  Waitable task(Args... args) {
    auto task = lib::task::create(args...);
    return Ctrl::task(task);
  }

  void forget(Waitable& w) {
    forgotten.push_back(std::move(w));
  }

  void forget(Waitable&& w) {
    forgotten.push_back(std::move(w));
  }

  void run();
  void run_decorated() { run(); }
};

#ifdef WINDOWS
  #define MAIN_NAME WinMain
#else
  #define MAIN_NAME main
#endif

#define MAIN_MACRO(CTRL) \
  void control( \
    lib::task::Context<QUEUE_INDEX_CONTROL_THREAD_ONLY> *ctx \
  ) { \
    CTRL ctrl { ctx }; \
    ctrl.run_decorated(); \
  } \
  \
  int MAIN_NAME() { \
    engine::startup(control); \
    return 0; \
  } \
