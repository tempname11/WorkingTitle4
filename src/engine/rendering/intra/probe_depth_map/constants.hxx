#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/constants.hxx>

namespace engine::rendering::intra::probe_depth_map {

const VkFormat FORMAT = VK_FORMAT_R16G16_SFLOAT; // :ProbeDepthFormat

// :ProbeDepthOctoSize
const uint32_t OCTOSIZE = 16;

const uint32_t WIDTH = (OCTOSIZE
  * PROBE_GRID_SIZE.x
  * PROBE_GRID_SIZE_Z_FACTORS.x
  * PROBE_CASCADE_COUNT_FACTORS.x
);
const uint32_t HEIGHT = (OCTOSIZE
  * PROBE_GRID_SIZE.y 
  * PROBE_GRID_SIZE_Z_FACTORS.y
  * PROBE_CASCADE_COUNT_FACTORS.y
);

} // namespace
