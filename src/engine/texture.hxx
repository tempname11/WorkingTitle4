#pragma once
#include "common/texture.hxx"
#include "session.hxx"

namespace engine::texture {
  const auto ALBEDO_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
  const auto NORMAL_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM; // SNORM doesn't work right, for some reason.
  const auto ROMEAO_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

  engine::common::texture::Data<uint8_t> load_rgba8(const char *filename);
}
