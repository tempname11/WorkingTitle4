#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/lib/guid.hxx>

namespace engine::system::grup::texture {

void deref(
  lib::GUID texture_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::MetaTextures> meta_textures
);

void reload(
  lib::GUID texture_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaTextures> meta_textures
);

lib::Task *load(
  std::string &path,
  VkFormat format,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaTextures> meta_textures,
  Use<engine::session::Data::GuidCounter> guid_counter,
  lib::GUID *out_texture_id
);

} // namespace
