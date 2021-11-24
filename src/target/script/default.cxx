#include <mutex>
#include <src/global.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/startup.hxx>
#include <src/engine/system/grup/group.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/session/data.hxx>
#include "session.inline.hxx"

void update_stuff(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<engine::session::Data::State> state
) {
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0, 0.0);
}

void CtrlSession::run() {
  if (1) {
    wait_for_signal(
      engine::system::artline::load(
        lib::cstr::from_static("out/build/x64-Debug/" "temple.art.dll"), // @Cleanup: path
        session, ctx
      )
    );
  }

  if (0) {
    std::string path = "assets/gi_test_0.grup";
    task(engine::system::grup::group::load(
      ctx,
      &path,
      session
    ));
    task(update_stuff, &session->state);
  }
}

MAIN_MACRO(CtrlSession);
