#pragma once
#include <glm/glm.hpp>

namespace engine {

// @Incomplete: we may need to calculate this depending on
// the available GPU memory, the number of memory types we need,
// and the `maxMemoryAllocationCount` limit.
const size_t ALLOCATOR_GPU_LOCAL_REGION_SIZE = 1024 * 1024 * 32;

const auto PROBE_WORLD_DELTA = glm::vec3(2.0);
const auto PROBE_GRID_SIZE = glm::uvec3(32, 32, 8);
const auto PROBE_GRID_SIZE_Z_FACTORS = glm::uvec2(4, 2);
const auto PROBE_RAY_COUNT = 64; // :DDGI_N_Rays 64
const auto PROBE_RAY_COUNT_FACTORS = glm::uvec2(8, 8);

const auto G2_TEXEL_SIZE = glm::uvec2(
  PROBE_GRID_SIZE.x * PROBE_GRID_SIZE_Z_FACTORS.x * PROBE_RAY_COUNT_FACTORS.x,
  PROBE_GRID_SIZE.y * PROBE_GRID_SIZE_Z_FACTORS.y * PROBE_RAY_COUNT_FACTORS.y
);

} // namespace
