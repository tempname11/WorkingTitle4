#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_new_frame( \
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, \
  usage::Full<SessionData::ImguiContext> imgui, \
  usage::Full<RenderingData::ImguiBackend> imgui_backend, \
  usage::Full<SessionData::GLFW> glfw /* hacky way to prevent KeyMods bug. */ \
)

TASK_DECL;
