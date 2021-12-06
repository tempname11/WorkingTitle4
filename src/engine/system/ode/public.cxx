#include <src/lib/single_allocator.hxx>
#include <src/engine/component.hxx>
#include "impl.hxx"

namespace engine::system::ode {

const double TIME_STEP_SEC = 0.01;
const size_t MAX_FRAME_STEPS = 5;

const size_t MAX_BODY_COMPONENTS = 1024 * 1024;
const size_t CONTACT_POINTS = 16;

void _ode_moved_callback(dBodyID body) {
  auto data = (ComponentBody *) dBodyGetData(body);
  if (data != nullptr) {
    data->updated_this_frame = true;
  }
}

void _handle_collisions(void* ptr, dGeomID ga, dGeomID gb) {
  auto it = (Impl *) ptr;
  dContact contacts[CONTACT_POINTS];
  auto n = dCollide(ga, gb, CONTACT_POINTS, &contacts[0].geom, sizeof(dContact));

  auto ba = dGeomGetBody(ga);
  auto bb = dGeomGetBody(gb);
  for (size_t i = 0; i < n; i++) {
    contacts[i].surface.mode = dContactApprox1;
    contacts[i].surface.mu = 0.5;

    auto joint = dJointCreateContact(it->world, it->collision_joints, &contacts[i]);
    dJointAttach(joint, ba, bb);
  }
}

void register_body(Impl* it, dBodyID body) {
  dBodySetData(
    body,
    &it->bodies->data[it->bodies->count]
  );
  dBodySetMovedCallback(body, _ode_moved_callback);
  lib::array::ensure_space(&it->bodies, 1);
  it->bodies->data[it->bodies->count++] = {
    .body = body,
  };
}

void init(Impl* out) {
  {
    auto result = dInitODE2(0);
    assert(result != 0);
  }

  auto world = dWorldCreate();
  dWorldSetGravity(world, 0, 0, -10);
  auto space = dSimpleSpaceCreate(0);
  auto collision_joints = dJointGroupCreate(0);

  *out = {
    .world = world,
    .space = space,
    .collision_joints = collision_joints,
    .bodies = lib::array::create<system::ode::ComponentBody>(
      lib::single_allocator::create(
        sizeof(system::ode::ComponentBody) * MAX_BODY_COMPONENTS
      ),
      0
    ),
  };
}

void deinit(Impl* it) {
  lib::array::destroy(it->bodies);
  dSpaceDestroy(it->space);
  dWorldDestroy(it->world);
  dCloseODE();
}

void update(Impl* it, double elapsed_sec) {
  // It's important this executed on the main thread, both for ODE thread safety,
  // and because of lack of synchronization for `it`.

  it->residual_elapsed_physics_sec += elapsed_sec;
  for (size_t i = 0; i < MAX_FRAME_STEPS; i++) {
    if (it->residual_elapsed_physics_sec > TIME_STEP_SEC) {
      it->residual_elapsed_physics_sec -= TIME_STEP_SEC;
      dSpaceCollide(it->space, it, _handle_collisions);
      dWorldQuickStep(it->world, TIME_STEP_SEC);
      dJointGroupEmpty(it->collision_joints);
    } else {
      break;
    }
  }
}

} // namespace
