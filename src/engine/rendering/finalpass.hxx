#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data.hxx"

void init_session_finalpass(
  engine::session::Vulkan::Finalpass *out,
  engine::session::Vulkan::Core *core
);

void deinit_session_finalpass(
  engine::session::Vulkan::Finalpass *it,
  engine::session::Vulkan::Core *core
);

void init_rendering_finalpass(
  engine::display::Data::Finalpass *out,
  engine::display::Data::Common *common,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::LBuffer *lbuffer,
  engine::display::Data::FinalImage *final_image,
  engine::session::Vulkan::Finalpass *s_finalpass,
  engine::session::Vulkan::Core *core
);

void deinit_rendering_finalpass(
  engine::display::Data::Finalpass *it,
  engine::session::Vulkan::Core *core
);