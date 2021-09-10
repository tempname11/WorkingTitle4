#include "defer.hxx"
#include "frame_setup_gpu_signal.hxx"

void signal_cleanup (
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData::InflightGPU> inflight_gpu,
  Own<uint8_t> inflight_index_saved
) {
  std::scoped_lock lock(inflight_gpu->mutex);
  assert(inflight_gpu->signals[*inflight_index_saved] != nullptr);
  inflight_gpu->signals[*inflight_index_saved] = nullptr;
  delete inflight_index_saved.ptr;
}

TASK_DECL {
  ZoneScoped;
  // don't use inflight_gpu->mutex, since out signal slot is currently unused
  assert(session->inflight_gpu.signals[frame_info->inflight_index] == nullptr);
  auto signal = lib::gpu_signal::create(
    gpu_signal_support.ptr,
    core->device,
    *frame_rendered_semaphore,
    frame_info->timeline_semaphore_value
  );
  auto inflight_index_saved = new uint8_t(frame_info->inflight_index); // frame_info will not be around!
  auto task_cleanup = defer(
    task::create(
      signal_cleanup,
      &session->inflight_gpu,
      inflight_index_saved
    )
  );
  session->inflight_gpu.signals[frame_info->inflight_index] = task_cleanup.second;
  lib::task::inject(ctx->runner, {
    task_cleanup.first
  }, {
    .new_dependencies = {
      { signal, task_cleanup.first }
    }
  });
}
