#include "frame_update.hxx"

TASK_DECL {
  ZoneScoped;

  // Note: a big part of "update" currently happens in glfwPollEvents.

  double elapsed_sec = double(frame_info->elapsed_ns) / (1000.0 * 1000.0 * 1000.0);
  if (elapsed_sec > 0) {
    session_state->debug_camera_prev = session_state->debug_camera;
    lib::debug_camera::update(
      &session_state->debug_camera,
      &update->debug_camera_input,
      elapsed_sec
    );
  }

  if (!session_state->ubo_flags.disable_eye_adaptation) {
    auto ptr = readback_data->luminance_pointers[frame_info->inflight_index];
    auto r = glm::detail::toFloat32(ptr[0]);
    auto g = glm::detail::toFloat32(ptr[1]);
    auto b = glm::detail::toFloat32(ptr[2]);

    // @Cleanup constants
    auto w = std::min(elapsed_sec / 2.0, 1.0);
    auto p = 4.0;
    session_state->luminance_moving_average = std::pow((0.0
      + (1.0 - w) * std::pow(session_state->luminance_moving_average, 1.0 / p)
      + /*****/ w * std::pow(r * 0.21 + g * 0.72 + b * 0.07, 1.0 / p)
    ), p);
  }
}
