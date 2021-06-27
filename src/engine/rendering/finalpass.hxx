#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
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
  RenderingData::Finalpass *out,
  RenderingData::Common *common,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan::Finalpass *s_finalpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_finalpass(
  RenderingData::Finalpass *it,
  SessionData::Vulkan::Core *core
);