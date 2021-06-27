#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_prepass(
  RenderingData::Prepass *it,
  VkShaderModule module_vert,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::GPass *s_gpass,
  SessionData::Vulkan::Core *core
);