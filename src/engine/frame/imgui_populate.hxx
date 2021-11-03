#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/system/grup/decl.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void imgui_populate(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display,
  Own<engine::session::Data::ImguiContext> imgui,
  Own<engine::misc::ImguiReactions> imgui_reactions,
  Use<engine::system::grup::MetaMeshes> meta_meshes,
  Use<engine::system::grup::MetaTextures> meta_textures,
  Own<engine::session::Data::State> state
);

} // namespace
