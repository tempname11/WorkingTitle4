#pragma once
#include <src/engine/component.hxx>
#include "decl.hxx"

namespace engine::system::entities {

void init(Impl *out);
void deinit(Impl *it);

struct added_component_t {
  component::type_t type;
  component::index_t ix;
};

void add(Impl *it, added_component_t *components, uint8_t count);

} // namespace
