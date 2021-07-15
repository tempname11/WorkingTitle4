#pragma once
#include <src/engine/session.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_populate( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<SessionData::ImguiContext> imgui, \
  usage::Some<SessionData::State> state \
)

TASK_DECL;
