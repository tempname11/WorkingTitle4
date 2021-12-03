#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data.hxx"
#include "../session/data/vulkan.hxx"

void init_session_prepass(
  engine::session::VulkanData::Prepass *out,
  engine::session::VulkanData::GPass *gpass,
  engine::session::VulkanData::Core *core,
  VkShaderModule module_vert
);

void deinit_session_prepass(
  engine::session::VulkanData::Prepass *it,
  engine::session::VulkanData::Core *core
);

void init_rendering_prepass(
  engine::display::Data::Prepass *out,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::session::VulkanData::Prepass *s_prepass,
  engine::session::VulkanData::Core *core
);

void deinit_rendering_prepass(
  engine::display::Data::Prepass *it,
  engine::session::VulkanData::Core *core
);