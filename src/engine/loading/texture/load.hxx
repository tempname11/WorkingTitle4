#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/common/texture.hxx>
#include "../texture.hxx"

namespace engine::loading::texture {

struct LoadData {
  lib::GUID texture_id;
  std::string path;
  VkFormat format;

  engine::common::texture::Data<uint8_t> the_texture;
  SessionData::Vulkan::Textures::Item texture_item;
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
);

void _load_init_image(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Ref<lib::Task> signal,
  Own<LoadData> data 
);

} // namespace
