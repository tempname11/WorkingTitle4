#include "after_inflight.hxx"

void after_inflight(
  lib::task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<lib::Task> task
) {
  ZoneScoped;

  std::scoped_lock lock(session->inflight_gpu.mutex);
  std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
  for (auto signal : session->inflight_gpu.signals) {
    dependencies.push_back({ signal, task.ptr });
  }

  lib::task::inject(ctx->runner, {
    task.ptr
  }, {
    .new_dependencies = dependencies,
  });
}