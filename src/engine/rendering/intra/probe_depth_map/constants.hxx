#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/constants.hxx>

namespace engine::rendering::intra::probe_depth_map {

const VkFormat FORMAT = VK_FORMAT_R16G16_SFLOAT; // :ProbeDepthFormat

// :ProbeDepthOctoSize
const uint32_t WIDTH = PROBE_GRID_SIZE.x * PROBE_GRID_SIZE_Z_FACTORS.x * 16;
const uint32_t HEIGHT = PROBE_GRID_SIZE.y * PROBE_GRID_SIZE_Z_FACTORS.y * 16;

} // namespace
