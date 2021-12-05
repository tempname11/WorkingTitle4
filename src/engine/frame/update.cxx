#include <src/engine/session/data/ode.hxx>
#include "update.hxx"
 
namespace engine::frame {

float halton(int i, int b) {
  float f = 1.0;
  float r = 0.0;
  while (i > 0) {
    f = f / float(b);
    r = r + f * float(i % b);
    i = i / b;
  }
  return r;
}

void handle_collisions(void* ptr, dGeomID ga, dGeomID gb) {
  auto data = (session::ODE_Data *) ptr;
  dContact contacts[16];
  auto n = dCollide(ga, gb, 16, &contacts[0].geom, sizeof(dContact));

  auto ba = dGeomGetBody(ga);
  auto bb = dGeomGetBody(gb);
  for (size_t i = 0; i < n; i++) {
    contacts[i].surface.mode = dContactApprox1;
    contacts[i].surface.mu = 0.5;

    auto joint = dJointCreateContact(data->world, data->collision_joints, &contacts[i]);
    dJointAttach(joint, ba, bb);
  }
}

void update(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, // main thread for ODE
  Use<misc::UpdateData> update,
  Ref<display::Data::FrameInfo> frame_info,
  Use<display::Data::Readback> readback_data,
  Own<session::Data::State> session_state,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  // Note: a big part of "update" currently happens in glfwPollEvents.

  double elapsed_sec = double(frame_info->elapsed_ns) / (1000.0 * 1000.0 * 1000.0);
  if (elapsed_sec > 0) {
    session_state->taa_jitter_offset_prev = session_state->taa_jitter_offset;
    if (session_state->ubo_flags.disable_TAA) {
      session_state->taa_jitter_offset = glm::vec2(0.0f);
    } else {
      auto t = glm::vec2(
        halton(frame_info->number % 192, 2),
        halton(frame_info->number % 192, 3)
      );
      session_state->taa_jitter_offset = t - 0.5f;
    }

    session_state->debug_camera_prev = session_state->debug_camera;
    lib::debug_camera::update(
      &session_state->debug_camera,
      &update->debug_camera_input,
      elapsed_sec,
      session_state->movement_speed
    );
  }

  if (!session_state->ubo_flags.disable_eye_adaptation) {
    auto ptr = readback_data->radiance_pointers[frame_info->inflight_index];
    auto r = glm::detail::toFloat32(ptr[0]);
    auto g = glm::detail::toFloat32(ptr[1]);
    auto b = glm::detail::toFloat32(ptr[2]);

    // @Cleanup constants
    auto w = std::min(elapsed_sec / 2.0, 1.0);
    auto p = 4.0;
    session_state->luminance_moving_average = std::max(
      0.01,
      std::pow((0.0
        + (1.0 - w) * std::pow(session_state->luminance_moving_average, 1.0 / p)
        + /*****/ w * std::pow(r * 0.21 + g * 0.72 + b * 0.07, 1.0 / p)
      ), p)
    );
  }

  { // ODE
    dSpaceCollide(session->ode->space, session->ode, handle_collisions);
    const double time_step = 0.01;
    dWorldQuickStep(session->ode->world, time_step);
    dJointGroupEmpty(session->ode->collision_joints);
  }

  {
    auto cs = session->ode->body_components;
    for (size_t i = 0; i < cs->count; i++) {
      cs->data[i].updated_this_frame = false;
      if (scene->items.size() > i) { // @Hack ultra hack
        auto r = dBodyGetRotation(cs->data[i].body);
        auto p = dBodyGetPosition(cs->data[i].body);
        scene->items[i].transform = {
          r[0], r[4], r[8], 0,
          r[1], r[5], r[9], 0,
          r[2], r[6], r[10], 0,
          /*
          r[0], r[1], r[2], 0,
          r[3], r[4], r[5], 0,
          r[6], r[7], r[8], 0,
          */
          /*
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          */
          p[0], p[1], p[2], 1,
        };
      }
    }
  }
}

} // namespace
