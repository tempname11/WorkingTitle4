#pragma once
#include <src/engine/session.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_populate( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Full<SessionData::ImguiContext> imgui, \
  usage::Full<engine::misc::ImguiReactions> imgui_reactions, \
  usage::Some<SessionData::State> state \
)

TASK_DECL;
