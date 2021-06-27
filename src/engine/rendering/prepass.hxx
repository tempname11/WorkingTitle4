#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_example_prepass(
  RenderingData::Example::Prepass *it,
  VkShaderModule module_vert,
  RenderingData::Example::ZBuffer *zbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::Example::GPass *s_gpass,
  SessionData::Vulkan::Core *core
);