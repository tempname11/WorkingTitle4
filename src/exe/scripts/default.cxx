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
  {
    wait_for_signal(
      engine::system::artline::load(
        "test.art.dll",
        session, ctx
      )
    );
  }
  {
    std::string path = "assets/vox/medieval_city_2/index.grup";
    task(engine::system::grup::group::load(
      ctx,
      &path,
      session
    ));
    task(update_stuff, &session->state);
  }
}

MAIN_MACRO(CtrlSession);
