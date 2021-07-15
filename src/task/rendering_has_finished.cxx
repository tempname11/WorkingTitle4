#include "rendering_has_finished.hxx"

TASK_DECL {
  ZoneScoped;
  lib::task::signal(ctx->runner, rendering_yarn_end.ptr);
}
