#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/constants.hxx>

namespace engine::rendering::intra::probe_light_map {

// @Note: if we use this format, need to check device compatibility.
const VkFormat FORMAT = VK_FORMAT_B10G11R11_UFLOAT_PACK32; // also used in compute shader
const uint32_t WIDTH = PROBE_GRID_SIZE.x * PROBE_GRID_SIZE_Z_FACTORS.x * 6;
const uint32_t HEIGHT = PROBE_GRID_SIZE.y * PROBE_GRID_SIZE_Z_FACTORS.y * 6;

} // namespace
