#include <mutex>
#include <src/global.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/startup.hxx>
#include <src/engine/loading/group.hxx>
#include <src/engine/session/setup.hxx>
#include "session.inline.hxx"

void update_stuff(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<engine::session::Data::State> state
) {
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0, 0.0);
}

void CtrlSession::run() {
  {
    std::string path = "assets/vox/medieval_city_2/index.grup";
    task(engine::loading::group::load(
      ctx,
      &path,
      session
    ));
    task(update_stuff, &session->state);
  }
}

MAIN_MACRO(CtrlSession);
