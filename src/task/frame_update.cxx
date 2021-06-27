#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

void frame_update(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<UpdateData> update,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<SessionData::State> session_state
) {
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
