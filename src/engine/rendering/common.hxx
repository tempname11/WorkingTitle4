#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data.hxx"

void claim_rendering_common(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::Common::Stakes *out
);

void init_rendering_common(
  engine::display::Data::Common::Stakes stakes,
  engine::display::Data::Common *out,
  engine::session::Vulkan::Core *core
);

void deinit_rendering_common(
  engine::display::Data::Common *it,
  engine::session::Vulkan::Core *core
);