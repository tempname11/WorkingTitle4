#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_new_frame( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Full<engine::session::Data::ImguiContext> imgui, \
  usage::Full<engine::display::Data::ImguiBackend> imgui_backend, \
  usage::Full<engine::session::Data::GLFW> glfw /* hacky way to prevent KeyMods bug. */ \
)

TASK_DECL;
