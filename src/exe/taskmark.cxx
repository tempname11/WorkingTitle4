#include <atomic>
#include <mutex>
#include <vector>
#include <src/global.hxx>

struct Counter {
  std::mutex mutex;
  std::atomic<int64_t> value = 0;
};

struct Session {
  const size_t p; // counters
  const size_t q; // tasks per iteration
  const size_t r; // iterations
  double t; // milliseconds to spin each task (on average)

  size_t iterations_left;
  std::vector<Counter> counters;
};

void mutate(
  lib::task::Context<0> *ctx,
  Use<Session> session,
  Own<Counter> counter_f,
  Use<Counter> counter_s
) {
  ZoneScopedC(0x00FF00);
  auto f_value = counter_f->value.load();
  std::scoped_lock lock(counter_s->mutex);
  auto s_value = counter_s->value.load();

  { ZoneScopedN("spin");
    using namespace std::chrono_literals;
    auto now = std::chrono::high_resolution_clock::now();
    auto delay = 2ms * session->t * rand() / RAND_MAX;
    while ((std::chrono::high_resolution_clock::now() - now) < delay) {
      std::this_thread::yield();
    }
  }

  assert(counter_f->value.load() == f_value);
  assert(counter_s->value.load() == s_value);

  int64_t delta = rand();
  counter_f->value.store(f_value + delta);
  counter_s->value.store(s_value - delta);
}

void session(
  lib::task::Context<0> *ctx,
  Own<Session> data
) {
  ZoneScopedC(0xFF0000);
  if (data->iterations_left == 0) {
    return;
  }
  data->iterations_left--;
  assert(data->p >= 2);
  assert(RAND_MAX >= data->p);
  std::vector<lib::Task *> tasks;
  for (size_t i = 0; i < data->q; i++) {
    auto p0 = rand() % data->p;
    auto p1 = rand() % data->p;
    if (p0 == p1) {
      p1 = (p1 + 1) % data->p;
    }
    tasks.push_back(lib::task::create(
      mutate,
      data.ptr,
      &data->counters[p0],
      &data->counters[p1]
    ));
  }
  tasks.push_back(lib::task::create(
    session,
    data.ptr
  ));
  lib::task::inject(ctx->runner, std::move(tasks));
}

int main() {
  const size_t num_queues = 1;
  const lib::task::QueueAccessFlags all_queue_access_flags = (1 << 0);

  auto num_threads = 8;
  size_t worker_count = num_threads - 1;
  std::vector<lib::task::QueueAccessFlags> worker_access_flags(worker_count, all_queue_access_flags);
  auto runner = lib::task::create_runner(1);

  size_t p = 80;
  size_t q = 800;
  size_t r = 1000;
  double t = 0.01;
  auto session_data = Session {
    .p = p, .q = q,
    .r = r, .t = t,
    .iterations_left = r,
    .counters = std::vector<Counter>(p),
  };

  lib::task::inject(runner, {
    lib::task::create(session, &session_data),
  });
  
  int64_t old_sum = 0;
  int64_t old_abs_sum = 0;
  for (auto &c : session_data.counters) {
    old_sum += c.value.load();
    old_abs_sum += abs(c.value.load());
  }
  LOG("old sum: {}", old_sum);
  LOG("old abs sum: {}", old_abs_sum);

  lib::task::run(runner, std::move(worker_access_flags), all_queue_access_flags);
  lib::task::discard_runner(runner);

  int64_t new_sum = 0;
  int64_t new_abs_sum = 0;
  for (auto &c : session_data.counters) {
    new_sum += c.value.load();
    new_abs_sum += abs(c.value.load());
  }
  LOG("new sum: {}", new_sum);
  LOG("new abs sum: {}", new_abs_sum);
  assert(old_sum == new_sum);

  return 0;
}