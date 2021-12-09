#pragma once
#include <ode/ode.h>
#include <src/engine/component.hxx>

namespace engine::system::ode {

struct Impl;
component::index_t register_body(
  Impl* it,
  component::ode_body::storage_t *cmp,
  dBodyID body
);
void init(Impl* out);
void deinit(Impl *it);
void update(Impl* it, double elapsed_sec);

}
