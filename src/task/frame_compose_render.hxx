#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_compose_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<engine::session::Data> session, \
  Ref<engine::display::Data> display, \
  usage::Some<engine::session::Vulkan::Core> core, \
  usage::Some<engine::display::Data::Presentation> presentation, \
  usage::Some<engine::display::Data::PresentationFailureState> presentation_failure_state, \
  usage::Some<engine::display::Data::SwapchainDescription> swapchain_description, \
  usage::Some<engine::display::Data::CommandPools> command_pools, \
  usage::Some<engine::display::Data::FrameInfo> frame_info, \
  usage::Some<engine::display::Data::FinalImage> final_image, \
  usage::Full<engine::misc::ComposeData> data \
)

TASK_DECL;
