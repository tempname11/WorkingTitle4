#pragma once
#include <ode/ode.h>

namespace engine::component::ode_body {

struct item_t {
  dBodyID body;
  bool updated_this_frame;
};

} // namespace
