#pragma once
#include <src/global.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void generate_render_list(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<session::Data> session,
  Use<session::VulkanData::Meshes> meshes,
  Use<session::VulkanData::Textures> textures,
  Use<system::entities::Impl> entities,
  Use<component::artline_model::storage_t> cmp_artline_model,
  Use<component::base_transform::storage_t> cmp_base_transform,
  Own<misc::RenderList> render_list
);

} // namespace
