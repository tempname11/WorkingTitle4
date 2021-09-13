#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
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
  engine::display::Data::Prepass *out,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::Prepass *s_prepass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_prepass(
  engine::display::Data::Prepass *it,
  SessionData::Vulkan::Core *core
);