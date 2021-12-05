#pragma once
#include <ode/ode.h>
#include <src/lib/base.hxx>

namespace engine::session {

struct BodyComponent {
  dBodyID body;
  bool updated_this_frame;
};

struct ODE_Data {
  dWorldID world;
  dSpaceID space;
  dJointGroupID collision_joints;

  lib::array_t<BodyComponent> *body_components;
};

}
