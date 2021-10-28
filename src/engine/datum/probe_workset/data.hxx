#pragma once
#include <vector>
#include <src/lib/gfx/allocator.hxx>

namespace engine::datum::probe_workset {

struct SData {
  std::vector<lib::gfx::allocator::Buffer> buffers_workset;
  std::vector<lib::gfx::allocator::Buffer> buffers_counter;
};

} // namespace