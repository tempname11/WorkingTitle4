#include <cassert>
#include <ode/ode.h>
#include <src/global.hxx>

struct HandlerData {
  dWorldID world;
  dJointGroupID collision_joints;
};

void handle_collisions(void* ptr, dGeomID ga, dGeomID gb) {
  auto data = (HandlerData *) ptr;
  dContact contacts[16];
  auto n = dCollide(ga, gb, 16, &contacts[0].geom, sizeof(dContact));

  auto ba = dGeomGetBody(ga);
  auto bb = dGeomGetBody(gb);
  for (size_t i = 0; i < n; i++) {
    contacts[i].surface.mode = 0;
    contacts[i].surface.mu = 0.5;

    auto joint = dJointCreateContact(data->world, data->collision_joints, &contacts[i]);
    dJointAttach(joint, ba, bb);
  }
}

int WinMain() {
  {
    auto result = dInitODE2(0);
    assert(result != 0);
  }
  auto world = dWorldCreate();
  dWorldSetGravity(world, 0, 0, -10);
  auto space = dSimpleSpaceCreate(0);

  auto sphere = dBodyCreate(world);
  dBodySetPosition(sphere, 0, 0, 100);
  dMass sphere_mass;
  dMassSetSphere(&sphere_mass, 1.0, 1.0);
  dBodySetMass(sphere, &sphere_mass);
  auto sphere_geom = dCreateSphere(space, 1.0);
  dGeomSetBody(sphere_geom, sphere);
  auto ground_geom = dCreatePlane(space, 0, 0, 1, 0);

  auto collision_joints = dJointGroupCreate(0);
  auto handler_data = HandlerData {
    .world = world,
    .collision_joints = collision_joints,
  };
  for (size_t i = 0; i < 1024; i++) {
    dSpaceCollide(space, &handler_data, handle_collisions);
    const double time_step = 0.01;
    dWorldQuickStep(world, time_step);
    dJointGroupEmpty(collision_joints);

    auto pos = dBodyGetPosition(sphere);
    LOG("Position is: ({}, {}, {})", pos[0], pos[1], pos[2]);

    dReal vel[3];
    dBodyGetPointVel(sphere, 0, 0, 0, vel);
    LOG("Velocity is: ({}, {}, {})", vel[0], vel[1], vel[2]);
  }

  dSpaceDestroy(space);
  dWorldDestroy(world);
  dCloseODE();

  return 0;
}