#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data.hxx"

void init_session_prepass(
  engine::session::Vulkan::Prepass *out,
  engine::session::Vulkan::GPass *gpass,
  engine::session::Vulkan::Core *core,
  VkShaderModule module_vert
);

void deinit_session_prepass(
  engine::session::Vulkan::Prepass *it,
  engine::session::Vulkan::Core *core
);

void init_rendering_prepass(
  engine::display::Data::Prepass *out,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::session::Vulkan::Prepass *s_prepass,
  engine::session::Vulkan::Core *core
);

void deinit_rendering_prepass(
  engine::display::Data::Prepass *it,
  engine::session::Vulkan::Core *core
);