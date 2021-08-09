#pragma once
#include <src/engine/rendering.hxx>
#include <src/engine/session.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_setup_gpu_signal( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Use<SessionData::Vulkan::Core> core, \
  Use<lib::gpu_signal::Support> gpu_signal_support, \
  Own<VkSemaphore> frame_rendered_semaphore, \
  Use<RenderingData::FrameInfo> frame_info \
)

TASK_DECL;
