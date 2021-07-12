#pragma once
#include "common/texture.hxx"
#include "session.hxx"

namespace engine::texture {
  const auto ALBEDO_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
  const size_t ALBEDO_TEXEL_SIZE = 4;

  const auto NORMAL_TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
  const size_t NORMAL_TEXEL_SIZE = 4;

  engine::common::texture::Data<uint8_t> load_rgba8(const char *filename);

  void prepare(
    engine::common::texture::Data<uint8_t> *data,
    engine::common::texture::GPU_Data *gpu_data,
    lib::gfx::multi_alloc::StakeBuffer *staging_stake,
    SessionData::Vulkan::Core *core,
    VkCommandBuffer cmd,
    VkQueue queue_work
  );
}
