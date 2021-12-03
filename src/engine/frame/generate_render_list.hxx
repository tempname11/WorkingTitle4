#pragma once
#include <src/global.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void generate_render_list(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::Scene> scene,
  Use<engine::session::VulkanData::Meshes> meshes,
  Use<engine::session::VulkanData::Textures> textures,
  Own<engine::misc::RenderList> render_list
);

} // namespace
