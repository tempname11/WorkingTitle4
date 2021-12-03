#pragma once
#include <ode/ode.h>

namespace engine::session {

struct ODE_Data {
  dWorldID world;
  dSpaceID space;
};

}
