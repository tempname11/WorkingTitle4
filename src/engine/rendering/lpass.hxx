#pragma once
#include <vulkan/vulkan.h>
#include "../session/data/vulkan.hxx"
#include "../display/data.hxx"

void init_session_lpass(
  engine::session::VulkanData::LPass *out,
  engine::session::VulkanData::Core *core
);

void deinit_session_lpass(
  engine::session::VulkanData::LPass *it,
  engine::session::VulkanData::Core *core
);

void claim_rendering_lpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::LPass::Stakes *out
);

void init_rendering_lpass(
  engine::display::Data::LPass *out,
  engine::display::Data::LPass::Stakes stakes,
  engine::display::Data::Helpers *helpers,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer,
  engine::display::Data::LBuffer *lbuffer,
  engine::session::VulkanData::LPass *s_lpass,
  engine::session::VulkanData::Core *core
);

void deinit_rendering_lpass(
  engine::display::Data::LPass *it,
  engine::session::VulkanData::Core *core
);