#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
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
  RenderingData::LPass::Stakes *out
);

void init_rendering_lpass(
  RenderingData::LPass *out,
  RenderingData::LPass::Stakes stakes,
  RenderingData::Common *common,
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::GBuffer *gbuffer,
  RenderingData::LBuffer *lbuffer,
  SessionData::Vulkan::LPass *s_lpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_lpass(
  RenderingData::LPass *it,
  SessionData::Vulkan::Core *core
);