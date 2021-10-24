#include <src/global.hxx>
#include "defer.hxx"

namespace lib {

void _defer(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<lib::Task> task
) {
  ZoneScoped;
  lib::task::inject(ctx->runner, { task.ptr });
}

void _defer_many(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Own<std::vector<lib::Task *>> tasks
) {
  ZoneScoped;
  lib::task::inject(ctx->runner, std::move(*tasks));
  delete tasks.ptr;
}

std::pair<lib::Task *, lib::Task *> defer(lib::Task *task) {
  auto task_defer = lib::task::create(_defer, task);
  return { task_defer, task };
}

std::pair<lib::Task *, nullptr_t> defer_many(std::vector<lib::Task *> *tasks) {
  auto task_defer = lib::task::create(_defer_many, tasks);
  return { task_defer, nullptr };
}

} // namespace