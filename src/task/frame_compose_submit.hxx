#pragma once
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_compose_submit( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<VkQueue> queue_work, \
  usage::Full<engine::display::Data::Presentation> presentation, \
  usage::Some<VkSemaphore> imgui_finished_semaphore, \
  usage::Some<engine::display::Data::FrameInfo> frame_info, \
  usage::Full<engine::misc::ComposeData> data \
)

TASK_DECL;
