#include "frame_update.hxx"

TASK_DECL {
  // @Note: part of update currently happens in glfwPollEvents
  ZoneScoped;
  double elapsed_sec = double(frame_info->elapsed_ns) / (1000 * 1000 * 1000);
  if (elapsed_sec > 0) {
    lib::debug_camera::update(
      &session_state->debug_camera,
      &update->debug_camera_input,
      elapsed_sec
    );
  }
}
