#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_loading_dynamic( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Use<SessionData::GuidCounter> guid_counter, \
  Own<SessionData::MetaMeshes> meta_meshes, \
  Own<SessionData::MetaTextures> meta_textures, \
  Use<engine::misc::ImguiReactions> imgui_reactions \
)

TASK_DECL;
