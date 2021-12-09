#pragma once
#include <src/lib/base.hxx>
#include <src/engine/component.hxx>
#include "decl.hxx"

namespace engine::system::entities {

struct by_type_t {
  uint64_t component_bit_mask;
  uint8_t component_count;
  uint8_t component_index_offsets[component::MAX_TYPES];
  lib::array_t<component::index_t> *indices;
};

struct Impl {
  lib::array_t<by_type_t> *types;
};

} // namespace
