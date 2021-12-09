#include <ode/ode.h>
#include <src/global.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/ode/public.hxx>
#include <src/engine/system/ode/impl.hxx>
#include <src/engine/session/data.hxx>
#include "session.inline.hxx"

void update_stuff(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, // main thread for ODE
  Ref<engine::session::Data> session
) {
  auto world = session->ode->world;
  auto space = session->ode->space;

  //!!
  for (size_t i = 0; i < 20; i++) {
    for (size_t j = 0; j < 25; j++) {
      auto body = dBodyCreate(world);
      auto ix = engine::system::ode::register_body(session->ode, body);

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
    auto result = engine::system::artline::load(
      lib::cstr::from_static("binaries/artline/temple.art.dll"),
      session, ctx
    );
    wait_for_signal(result.completed);
  }

  if (1) {
    auto result = engine::system::artline::load(
      lib::cstr::from_static("binaries/artline/test.art.dll"),
      session, ctx
    );
    wait_for_signal(result.completed);
    task(update_stuff, session);
  }

  if (1) {
    auto result = engine::system::artline::load(
      lib::cstr::from_static("binaries/artline/ground.art.dll"),
      session, ctx
    );
    wait_for_signal(result.completed);
  }
}

MAIN_MACRO(CtrlSession);
