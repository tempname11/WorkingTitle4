#pragma once
#include <vulkan/vulkan.h>

namespace engine::rendering::intra::probe_light_map {

// @Note: if we use this format, need to check device compatibility.
const VkFormat FORMAT = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
const uint32_t WIDTH = 2048;
const uint32_t HEIGHT = 2048;

} // namespace
