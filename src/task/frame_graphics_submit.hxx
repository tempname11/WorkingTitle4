#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_graphics_submit( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<VkQueue> queue_work, \
  usage::Some<VkSemaphore> example_finished_semaphore, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<engine::misc::GraphicsData> data \
)

TASK_DECL;
