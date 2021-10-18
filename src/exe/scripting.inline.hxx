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

void release_semaphore(
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
    auto after = lib::task::create(release_semaphore, semaphore);
    lib::task::inject(ctx->runner, {}, {
      .new_dependencies = {
        { signal, after },
      },
    });
    return Waitable(semaphore);
  }

  Waitable add(lib::Task *task) {
    auto semaphore = new std::binary_semaphore(0);
    auto after = lib::task::create(release_semaphore, semaphore);
    lib::task::inject(ctx->runner, { task }, {
      .new_dependencies = {
        { task, after },
      },
    });
    return Waitable(semaphore);
  }

  template <typename... Args>
  Waitable create(Args... args) {
    auto task = lib::task::create(args...);
    return add(task);
  }

  void forget(Waitable& w) {
    forgotten.push_back(std::move(w));
  }

  void forget(Waitable&& w) {
    forgotten.push_back(std::move(w));
  }

  void run();
};

void control(
  lib::task::Context<QUEUE_INDEX_CONTROL_THREAD_ONLY> *ctx
) {
  Ctrl ctrl { ctx };
  ctrl.run();
}

#ifdef WINDOWS
  int WinMain() {
    engine::startup(control);
    return 0;
  }
#else
  int main() {
    engine::startup(control);
    return 0;
  }
#endif
