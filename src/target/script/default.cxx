#include <ode/ode.h>
#include <src/global.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/grup/group.hxx>
#include <src/engine/system/ode/public.hxx>
#include <src/engine/system/ode/impl.hxx>
#include <src/engine/session/data.hxx>
#include "session.inline.hxx"

void update_stuff(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, // main thread for ODE
  Ref<engine::session::Data> session,
  Own<engine::session::Data::Scene> scene 
) {
  auto world = session->ode->world;
  auto space = session->ode->space;

  //!!
  for (size_t i = 0; i < 20; i++) {
    for (size_t j = 0; j < 25; j++) {
      auto body = dBodyCreate(world);
      engine::system::ode::register_body(session->ode, body);
      dBodySetPosition(
        body,
        (int(i) - 5) * 0.4,
        (int(j) - 5) * 0.4,
        1 + 2 * i + 40 * j
      );

      auto geom = dCreateBox(space, 2.0, 2.0, 2.0);
      dGeomSetBody(geom, body);

      dMass mass;
      dMassSetBox(&mass, 1.0, 2.0, 2.0, 2.0);
      dBodySetMass(body, &mass);
    }
  }

  auto ground_geom = dCreatePlane(space, 0, 0, 1, 0);
}

void CtrlSession::run() {
  if (0) {
    wait_for_signal(
      engine::system::artline::load(
        lib::cstr::from_static("binaries/artline/temple.art.dll"),
        session, ctx
      )
    );
  }

  if (1) {
    wait_for_signal(
      engine::system::artline::load(
        lib::cstr::from_static("binaries/artline/test.art.dll"),
        session, ctx
      )
    );
    task(update_stuff, session, &session->scene);
  }

  if (1) {
    wait_for_signal(
      engine::system::artline::load(
        lib::cstr::from_static("binaries/artline/ground.art.dll"),
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
  }
}

MAIN_MACRO(CtrlSession);
