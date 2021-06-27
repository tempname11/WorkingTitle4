#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_example_gpass(
  RenderingData::Example::GPass *it,
  VkDescriptorPool common_descriptor_pool,
  VkShaderModule module_vert,
  VkShaderModule module_frag,
  RenderingData::Example::ZBuffer *zbuffer,
  RenderingData::Example::GBuffer *gbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::Example::GPass *s_gpass,
  SessionData::Vulkan::Core *core
);