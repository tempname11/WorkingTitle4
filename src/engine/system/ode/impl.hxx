#pragma once
#include <ode/ode.h>
#include <src/lib/base.hxx>

namespace engine::system::ode {

struct ComponentBody {
  dBodyID body;
  bool updated_this_frame;
};

struct Impl {
  dWorldID world;
  dSpaceID space;
  dJointGroupID collision_joints;

  lib::array_t<ComponentBody> *bodies;

  double residual_elapsed_physics_sec;
};

}
