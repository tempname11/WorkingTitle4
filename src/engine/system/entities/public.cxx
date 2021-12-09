#pragma once
#include <src/lib/base.hxx>
#include "impl.hxx"

namespace engine::system::entities {

void init(Impl *out) {
  *out = {
    .types = lib::array::create<by_type_t>(lib::allocator::crt, 0),
  };
}

void deinit(Impl *it) {
  lib::array::destroy(it->types);
}

} // namespace
