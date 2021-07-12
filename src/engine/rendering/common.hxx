#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void claim_rendering_common(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  RenderingData::Common::Stakes *out
);

void init_rendering_common(
  RenderingData::Common::Stakes stakes,
  RenderingData::Common *out,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_common(
  RenderingData::Common *it,
  SessionData::Vulkan::Core *core
);