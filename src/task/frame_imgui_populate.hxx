#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_imgui_populate( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<engine::session::Data> session, \
  Own<engine::session::Data::ImguiContext> imgui, \
  Own<engine::misc::ImguiReactions> imgui_reactions, \
  Use<engine::session::Data::MetaMeshes> meta_meshes, \
  Use<engine::session::Data::MetaTextures> meta_textures, \
  Own<engine::session::Data::State> state \
)

TASK_DECL;
