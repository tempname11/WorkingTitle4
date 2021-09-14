#pragma once

namespace engine {

// @Incomplete: we may need to calculate this depending on
// the available GPU memory, the number of memory types we need,
// and the `maxMemoryAllocationCount` limit.
const size_t ALLOCATOR_GPU_LOCAL_REGION_SIZE = 1024 * 1024 * 32;

} // namespace
