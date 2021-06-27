#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_lpass(
  RenderingData::LPass *it,
  RenderingData::Common *common,
  RenderingData::GPass *gpass,
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::GBuffer *gbuffer,
  RenderingData::LBuffer *lbuffer,
  SessionData::Vulkan *vulkan
);