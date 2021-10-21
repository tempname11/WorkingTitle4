#pragma once
#include <glm/glm.hpp>

namespace engine {

// @Incomplete: we may need to calculate this depending on
// the available GPU memory, the number of memory types we need,
// and the `maxMemoryAllocationCount` limit.
const size_t ALLOCATOR_GPU_LOCAL_REGION_SIZE = 1024 * 1024 * 32;

constexpr auto PROBE_WORLD_DELTA_C0 = glm::vec3(1.0f);

constexpr auto PROBE_GRID_SIZE_Z_FACTORS = glm::uvec2(8, 8);
constexpr auto PROBE_GRID_SIZE = glm::uvec3(64, 64, 64);
static_assert(
  PROBE_GRID_SIZE.z ==
  PROBE_GRID_SIZE_Z_FACTORS.x * PROBE_GRID_SIZE_Z_FACTORS.y
);

constexpr auto PROBE_RAY_COUNT = 64; // :GI_N_Rays
constexpr auto PROBE_RAY_COUNT_FACTORS = glm::uvec2(8, 8);
static_assert(
  PROBE_RAY_COUNT ==
  PROBE_RAY_COUNT_FACTORS.x * PROBE_RAY_COUNT_FACTORS.y
);

constexpr size_t PROBE_CASCADE_COUNT = 4;
constexpr auto PROBE_CASCADE_COUNT_FACTORS = glm::uvec2(2, 2);
static_assert(
  PROBE_CASCADE_COUNT ==
  PROBE_CASCADE_COUNT_FACTORS.x * PROBE_CASCADE_COUNT_FACTORS.y
);

const auto G2_TEXEL_SIZE = (uint32_t(1)
  * PROBE_RAY_COUNT_FACTORS
  * glm::uvec2(PROBE_GRID_SIZE.x, PROBE_GRID_SIZE.y)
  * PROBE_GRID_SIZE_Z_FACTORS
  * PROBE_CASCADE_COUNT_FACTORS
);

} // namespace
