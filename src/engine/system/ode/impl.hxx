#pragma once
#include <src/lib/base.hxx>

namespace engine::system::ode {

struct Impl {
  dWorldID world;
  dSpaceID space;
  dJointGroupID collision_joints;

  double residual_elapsed_physics_sec;
};

} // namespace
