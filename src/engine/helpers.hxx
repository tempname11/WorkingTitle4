#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data.hxx"

namespace engine::helpers {

void claim(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::Helpers::Stakes *out
);

void init(
  engine::display::Data::Helpers::Stakes stakes,
  engine::display::Data::Helpers *out,
  engine::session::Vulkan::Core *core
);

void deinit(
  engine::display::Data::Helpers *it,
  engine::session::Vulkan::Core *core
);

} // namespace