#include "after_inflight.hxx"

void after_inflight(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::None<task::Task> task
) {
  ZoneScoped;

  std::scoped_lock lock(inflight_gpu->mutex);
  std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
  for (auto signal : inflight_gpu->signals) {
    dependencies.push_back({ signal, task.ptr });
  }

  task::inject(ctx->runner, {
    task.ptr
  }, {
    .new_dependencies = dependencies,
  });
}