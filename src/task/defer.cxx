#include "defer.hxx"

void defer(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::None<task::Task> task
) {
  ZoneScoped;
  task::inject(ctx->runner, { task.ptr });
}

void defer_many(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<std::vector<task::Task *>> tasks // use None instead?
) {
  ZoneScoped;
  task::inject(ctx->runner, std::move(*tasks));
  delete tasks.ptr;
}