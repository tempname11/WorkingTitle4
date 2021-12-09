#pragma once
#include <cstdint>
#include <src/lib/flat32.hxx>

namespace engine::component {

using index_t = uint32_t;
const index_t MAX_INDEX = ~uint32_t(0);

enum struct type_t {
  artline_model,
  base_transform,
  ode_body,
  _count,
};

static_assert(int(type_t::_count) <= 64);

namespace artline_model { struct item_t; using storage_t = lib::flat32_t<item_t>; }
namespace base_transform { struct item_t; using storage_t = lib::flat32_t<item_t>; }

} // namespace
