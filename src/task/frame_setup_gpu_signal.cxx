#include "defer.hxx"
#include "frame_setup_gpu_signal.hxx"

void signal_cleanup (
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::Full<uint8_t> inflight_index_saved
) {
  std::scoped_lock lock(inflight_gpu->mutex);
  assert(inflight_gpu->signals[*inflight_index_saved] != nullptr);
  DBG("signal_cleanup {}", (void*)inflight_gpu->signals[*inflight_index_saved]);
  inflight_gpu->signals[*inflight_index_saved] = nullptr;
  delete inflight_index_saved.ptr;
}

TASK_DECL {
  ZoneScoped;
  // don't use inflight_gpu->mutex, since out signal slot is currently unused
  assert(inflight_gpu->signals[frame_info->inflight_index] == nullptr);
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
      inflight_gpu.ptr,
      inflight_index_saved
    )
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    inflight_gpu->signals[frame_info->inflight_index] = task_cleanup.second; // not the signal itself, on purpose
  }
  DBG("setup_signal {} for frame {}", (void*)task_cleanup.second, frame_info->number);
  lib::task::inject(ctx->runner, {
    task_cleanup.first
  }, {
    .new_dependencies = {
      { signal, task_cleanup.first }
    }
  });
}
