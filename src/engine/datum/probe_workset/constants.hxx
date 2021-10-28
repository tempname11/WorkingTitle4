#pragma once
#include <src/engine/constants.hxx>

namespace engine::datum::probe_workset {

const uint32_t MAX_COUNT = (1
  * PROBE_GRID_SIZE.x
  * PROBE_GRID_SIZE.y
  * PROBE_GRID_SIZE.z
);

} // namespace
