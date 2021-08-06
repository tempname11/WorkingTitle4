#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_loading_dynamic( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::None<SessionData> session, \
  usage::Some<SessionData::GuidCounter> guid_counter, \
  usage::Full<SessionData::MetaMeshes> meta_meshes, \
  usage::Some<engine::misc::ImguiReactions> imgui_reactions, \
  usage::None<RenderingData::InflightGPU> inflight_gpu \
)

TASK_DECL;
