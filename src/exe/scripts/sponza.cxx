#include <mutex>
#include <src/global.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/startup.hxx>
#include <src/engine/loading/group.hxx>
#include <src/engine/session/setup.hxx>
#include "session.inline.hxx"

void update_camera(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<engine::session::Data::State> state
) {
  state->debug_camera.position = glm::vec3(5.0f, 0.0f, 5.0f);
  state->debug_camera.lon_lat = glm::vec2(3.0f, 0.0f);
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0, 0.0);
}

void CtrlSession::run() {
  {
    std::string path = "assets/sponza-fixed/index.grup";
    task(engine::loading::group::load(
      ctx,
      &path,
      session
    ));
    task(update_camera, &session->state);
  }

  //advance_frames(20);
  //std::string path = "tmp/sponza.bmp";
  //screenshot_next_frame(path);
  //advance_frames(1);
}

MAIN_MACRO(CtrlSession);
