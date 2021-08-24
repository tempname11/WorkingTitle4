#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "texture.hxx"

namespace engine::texture {

engine::common::texture::Data<uint8_t> load_rgba8(const char *filename) {
  int width, height, _channels = 4;
  auto data = stbi_load(filename, &width, &height, &_channels, 4);
  assert(data != nullptr);
  return engine::common::texture::Data<uint8_t> {
    .data = data,
    .width = width,
    .height = height,
    .channels = 4,
    .computed_mip_levels = lib::gfx::utilities::mip_levels(width, height),
  };
}

} // namespace
