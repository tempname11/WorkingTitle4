#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_example_lpass(
  RenderingData::Example::LPass *it,
  RenderingData::Example::GPass *gpass, // a bit dirty to depend on this
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::Example::ZBuffer *zbuffer,
  RenderingData::Example::GBuffer *gbuffer,
  RenderingData::Example::LBuffer *lbuffer,
  SessionData::Vulkan *vulkan
);