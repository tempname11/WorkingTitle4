#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/constants.hxx>

namespace engine::datum::probe_irradiance {

const VkFormat FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT; // :ProbeIrradianceFormat

// :ProbeOctoSize
const uint32_t OCTOSIZE = 8;

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
