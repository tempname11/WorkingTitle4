#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Full<SessionData::ImguiContext> imgui_context, \
  usage::Full<RenderingData::ImguiBackend> imgui_backend, \
  usage::Full<SessionData::GLFW> glfw, /* hacky way to prevent KeyMods bug. */ \
  usage::Some<RenderingData::SwapchainDescription> swapchain_description, \
  usage::Some<RenderingData::CommandPools> command_pools, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Full<engine::misc::ImguiData> data \
)

TASK_DECL;
