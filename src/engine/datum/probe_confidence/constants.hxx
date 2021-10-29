#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/constants.hxx>

namespace engine::datum::probe_confidence {

const VkFormat FORMAT = VK_FORMAT_R16G16B16A16_UINT; // :ProbeConfidenceFormat

const uint32_t WIDTH = (1
  * PROBE_GRID_SIZE.x
  * PROBE_GRID_SIZE_Z_FACTORS.x
  * PROBE_CASCADE_COUNT_FACTORS.x
);
const uint32_t HEIGHT = (1
  * PROBE_GRID_SIZE.y 
  * PROBE_GRID_SIZE_Z_FACTORS.y
  * PROBE_CASCADE_COUNT_FACTORS.y
);

} // namespace
