#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_setup_gpu_signal( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<engine::session::Data> session, \
  Use<engine::session::Vulkan::Core> core, \
  Use<lib::gpu_signal::Support> gpu_signal_support, \
  Own<VkSemaphore> frame_finished_semaphore, \
  Use<engine::display::Data::FrameInfo> frame_info \
)

TASK_DECL;
