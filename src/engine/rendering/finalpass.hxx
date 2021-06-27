#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_finalpass(
  RenderingData::Finalpass *it,
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan *vulkan
);