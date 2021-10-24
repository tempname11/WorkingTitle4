#include "rendering_has_finished.hxx"

void rendering_has_finished(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::Task> rendering_yarn_end
) {
  ZoneScoped;
  lib::task::signal(ctx->runner, rendering_yarn_end.ptr);
}
