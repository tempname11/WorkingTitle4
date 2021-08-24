#pragma once
#include <src/engine/session.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_populate( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Own<SessionData::ImguiContext> imgui, \
  Own<engine::misc::ImguiReactions> imgui_reactions, \
  Use<SessionData::MetaMeshes> meta_meshes, \
  Use<SessionData::MetaTextures> meta_textures, \
  Own<SessionData::State> state \
)

TASK_DECL;
