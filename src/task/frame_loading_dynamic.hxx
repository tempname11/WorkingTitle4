#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_loading_dynamic( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::None<SessionData> session, \
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns, \
  usage::Full<SessionData::Groups> groups, \
  usage::Some<RenderingData::InflightGPU> inflight_gpu, \
  usage::Some<engine::misc::ImguiReactions> imgui_reactions \
)

TASK_DECL;
