#include "task.hxx"

void rendering_has_finished(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> rendering_yarn_end
) {
  ZoneScoped;
  lib::task::signal(ctx->runner, rendering_yarn_end.ptr);
}
