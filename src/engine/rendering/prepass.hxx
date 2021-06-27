#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_session_prepass(
  SessionData::Vulkan::Prepass *out,
  SessionData::Vulkan::GPass *gpass,
  SessionData::Vulkan::Core *core,
  VkShaderModule module_vert
);

void deinit_session_prepass(
  SessionData::Vulkan::Prepass *it,
  SessionData::Vulkan::Core *core
);

void init_rendering_prepass(
  RenderingData::Prepass *out,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::Prepass *s_prepass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_prepass(
  RenderingData::Prepass *it,
  SessionData::Vulkan::Core *core
);