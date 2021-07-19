#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_compose_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Some<RenderingData::Presentation> presentation, \
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state, \
  usage::Some<RenderingData::SwapchainDescription> swapchain_description, \
  usage::Some<RenderingData::CommandPools> command_pools, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Some<RenderingData::FinalImage> final_image, \
  usage::Full<engine::misc::ComposeData> data \
)

TASK_DECL;
