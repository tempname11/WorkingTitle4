#pragma once
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_submit( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<VkQueue> queue_work, \
  usage::Some<VkSemaphore> graphics_finished_semaphore, \
  usage::Some<VkSemaphore> imgui_finished_semaphore, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<engine::misc::ImguiData> data \
)

TASK_DECL;