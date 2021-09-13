#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void rendering_imgui_setup_cleanup( \
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Full<engine::display::Data::ImguiBackend> imgui_backend \
)

TASK_DECL;
