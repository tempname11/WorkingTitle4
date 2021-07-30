#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/lib/guid.hxx>

namespace engine::loading::texture {

void deref(
  lib::GUID texture_id,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures
);

lib::Task *load(
  std::string &path,
  VkFormat format,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaTextures> meta_textures,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_texture_id
);

} // namespace