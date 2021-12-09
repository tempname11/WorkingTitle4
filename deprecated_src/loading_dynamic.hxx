#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/system/grup/decl.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void loading_dynamic(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Own<engine::system::grup::MetaMeshes> meta_meshes,
  Own<engine::system::grup::MetaTextures> meta_textures,
  Use<engine::misc::ImguiReactions> imgui_reactions
);

} // namespace
