#pragma once
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_compose_submit( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<VkQueue> queue_work, \
  usage::Full<RenderingData::Presentation> presentation, \
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state, \
  usage::Some<VkSemaphore> frame_rendered_semaphore, \
  usage::Some<VkSemaphore> imgui_finished_semaphore, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<ComposeData> data \
)

TASK_DECL;
