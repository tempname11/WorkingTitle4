#include <src/global.hxx>
#include "defer.hxx"

namespace lib {

void _defer(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<lib::Task> task
) {
  ZoneScoped;
  ctx->new_tasks.insert(
    ctx->new_tasks.end(),
    { task.ptr }
  );
}

void no_op(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx
) {
  ZoneScoped;
};

void _defer_many(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<std::vector<lib::Task *>> tasks,
  Ref<lib::Task> after_all
) {
  ZoneScoped;
  for (auto task : *tasks) {
    ctx->new_tasks.insert(
      ctx->new_tasks.end(),
      { task }
    );
    ctx->new_dependencies.insert(
      ctx->new_dependencies.end(),
      { { task, after_all.ptr } }
    );
  }
  ctx->new_tasks.insert(
    ctx->new_tasks.end(),
    { after_all.ptr }
  );
  // @Cleanup: use less inserts!
  delete tasks.ptr;
}

std::pair<lib::Task *, lib::Task *> defer(lib::Task *task) {
  auto task_defer = lib::task::create(_defer, task);
  return { task_defer, task };
}

std::pair<lib::Task *, lib::Task *> defer_many(std::vector<lib::Task *> *tasks) {
  auto task_no_op = lib::task::create(no_op);
  auto task_defer = lib::task::create(_defer_many, tasks, task_no_op);
  return { task_defer, task_no_op };
}

} // namespace