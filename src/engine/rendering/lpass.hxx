#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session.hxx"

void init_session_lpass(
  SessionData::Vulkan::LPass *out,
  SessionData::Vulkan::Core *core
);

void deinit_session_lpass(
  SessionData::Vulkan::LPass *it,
  SessionData::Vulkan::Core *core
);

void claim_rendering_lpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::LPass::Stakes *out
);

void init_rendering_lpass(
  engine::display::Data::LPass *out,
  engine::display::Data::LPass::Stakes stakes,
  engine::display::Data::Common *common,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer,
  engine::display::Data::LBuffer *lbuffer,
  SessionData::Vulkan::LPass *s_lpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_lpass(
  engine::display::Data::LPass *it,
  SessionData::Vulkan::Core *core
);