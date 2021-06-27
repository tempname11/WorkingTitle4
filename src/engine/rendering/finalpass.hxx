#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

void init_example_finalpass(
  RenderingData::Example::Finalpass *it,
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::Example::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan *vulkan
);