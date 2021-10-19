#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_loading_dynamic( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<engine::session::Data> session, \
  Use<engine::session::Data::GuidCounter> guid_counter, \
  Own<engine::session::Data::MetaMeshes> meta_meshes, \
  Own<engine::session::Data::MetaTextures> meta_textures, \
  Use<engine::misc::ImguiReactions> imgui_reactions \
)

TASK_DECL;
