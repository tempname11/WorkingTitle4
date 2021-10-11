#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session.hxx"

void init_session_finalpass(
  SessionData::Vulkan::Finalpass *out,
  SessionData::Vulkan::Core *core
);

void deinit_session_finalpass(
  SessionData::Vulkan::Finalpass *it,
  SessionData::Vulkan::Core *core
);

void init_rendering_finalpass(
  engine::display::Data::Finalpass *out,
  engine::display::Data::Common *common,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::LBuffer *lbuffer,
  engine::display::Data::FinalImage *final_image,
  SessionData::Vulkan::Finalpass *s_finalpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_finalpass(
  engine::display::Data::Finalpass *it,
  SessionData::Vulkan::Core *core
);