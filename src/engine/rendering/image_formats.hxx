#pragma once
#include <vulkan/vulkan.h>

const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat ZBUFFER_FORMAT = VK_FORMAT_D32_SFLOAT;
const VkFormat ZBUFFER2_FORMAT = VK_FORMAT_R16_SFLOAT; // also used in shader!
const VkFormat GBUFFER_CHANNEL0_FORMAT = VK_FORMAT_R16G16B16A16_SNORM; // also used in shader!
const VkFormat GBUFFER_CHANNEL1_FORMAT = VK_FORMAT_R8G8B8A8_UNORM; // also used in shader!
const VkFormat GBUFFER_CHANNEL2_FORMAT = VK_FORMAT_R8G8B8A8_UNORM; // also used in shader!
const VkFormat LBUFFER_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT; // :LBuffer_Format
const VkFormat LBUFFER2_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT; // :LBuffer2_Format
const VkFormat FINAL_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
