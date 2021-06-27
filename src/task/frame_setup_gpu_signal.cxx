#include <src/engine/rendering.hxx>
#include <src/engine/session.hxx>
#include "task.hxx"

void signal_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::Full<uint8_t> inflight_index_saved
) {
  std::scoped_lock lock(inflight_gpu->mutex);
  assert(inflight_gpu->signals[*inflight_index_saved] != nullptr);
  inflight_gpu->signals[*inflight_index_saved] = nullptr;
  delete inflight_index_saved.ptr;
}

void frame_setup_gpu_signal(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<lib::gpu_signal::Support> gpu_signal_support,
  usage::Full<VkSemaphore> frame_rendered_semaphore,
  usage::Full<RenderingData::InflightGPU> inflight_gpu,
  usage::Some<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  // don't use inflight_gpu->mutex, since out signal slot is currently unusedk
  assert(inflight_gpu->signals[frame_info->inflight_index] == nullptr);
  auto signal = lib::gpu_signal::create(
    gpu_signal_support.ptr,
    core->device,
    *frame_rendered_semaphore,
    frame_info->timeline_semaphore_value
  );
  auto inflight_index_saved = new uint8_t(frame_info->inflight_index); // frame_info will not be around!
  auto task_cleanup = task::create(signal_cleanup, inflight_gpu.ptr, inflight_index_saved);
  auto task_deferred_cleanup = task::create(defer, task_cleanup);
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    inflight_gpu->signals[frame_info->inflight_index] = task_cleanup; // not the signal itself, on purpose
  }
  lib::task::inject(ctx->runner, {
    task_deferred_cleanup
  }, {
    .new_dependencies = {
      { signal, task_deferred_cleanup }
    }
  });
}
