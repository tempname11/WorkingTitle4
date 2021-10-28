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
  //state->debug_camera.position = glm::vec3(5.0f, 0.0f, 5.0f);
  //state->debug_camera.lon_lat = glm::vec2(3.0f, 0.0f);
  state->debug_camera.position = glm::vec3(0.0f, 150.0f, -50.0f);
  state->debug_camera.lon_lat = glm::vec2(2.2f, 0.0f);
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0, 0.0);
}

void CtrlSession::run() {
  {
    std::string path = "assets/hogwarts/index.grup";
    //std::string path = "assets/sponza-fixed/index.grup";
    //std::string path = "assets/gi_test_0.grup";
    task(engine::loading::group::load(
      ctx,
      &path,
      session
    ));
    task(update_camera, &session->state);
  }
}

MAIN_MACRO(CtrlSession);
