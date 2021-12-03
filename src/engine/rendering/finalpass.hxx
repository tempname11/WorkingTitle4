#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data/vulkan.hxx"

void init_session_finalpass(
  engine::session::VulkanData::Finalpass *out,
  engine::session::VulkanData::Core *core
);

void deinit_session_finalpass(
  engine::session::VulkanData::Finalpass *it,
  engine::session::VulkanData::Core *core
);

void init_rendering_finalpass(
  engine::display::Data::Finalpass *out,
  engine::display::Data::Helpers *helpers,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::LBuffer *lbuffer,
  engine::display::Data::FinalImage *final_image,
  engine::session::VulkanData::Finalpass *s_finalpass,
  engine::session::VulkanData::Core *core
);

void deinit_rendering_finalpass(
  engine::display::Data::Finalpass *it,
  engine::session::VulkanData::Core *core
);