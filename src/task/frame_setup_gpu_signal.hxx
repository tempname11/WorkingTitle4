#pragma once
#include <src/engine/rendering.hxx>
#include <src/engine/session.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_setup_gpu_signal( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Some<lib::gpu_signal::Support> gpu_signal_support, \
  usage::Full<VkSemaphore> frame_rendered_semaphore, \
  usage::Full<RenderingData::InflightGPU> inflight_gpu, \
  usage::Some<RenderingData::FrameInfo> frame_info \
)

TASK_DECL;
