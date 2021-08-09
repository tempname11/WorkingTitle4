#include "after_inflight.hxx"

void after_inflight(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<SessionData> session,
  Ref<task::Task> task
) {
  ZoneScoped;

  std::scoped_lock lock(session->inflight_gpu.mutex);
  std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
  for (auto signal : session->inflight_gpu.signals) {
    dependencies.push_back({ signal, task.ptr });
  }

  task::inject(ctx->runner, {
    task.ptr
  }, {
    .new_dependencies = dependencies,
  });
}