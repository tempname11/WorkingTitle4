#include <ode/ode.h>
#include <src/global.hxx>
#include <src/engine/component.hxx>
#include <src/engine/component/artline_model.hxx>
#include <src/engine/component/base_transform.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/ode/public.hxx>
#include <src/engine/system/ode/impl.hxx>
#include <src/engine/system/entities/public.hxx>
#include <src/engine/session/data.hxx>
#include "session.inline.hxx"

void update_stuff(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx, // main thread for ODE
  Own<engine::component::artline_model::storage_t> cmp_artline_model,
  Own<engine::component::base_transform::storage_t> cmp_base_transform,
  Own<engine::component::ode_body::storage_t> cmp_ode_body,
  Own<engine::system::entities::Impl> entities,
  Ref<engine::system::artline::Model> model,
  Ref<engine::session::Data> session
) {
  auto world = session->ode->world;
  auto space = session->ode->space;

  auto model_ix = lib::flat32::add(cmp_artline_model.ptr);
  cmp_artline_model->values->data[model_ix] = (
    engine::component::artline_model::item_t {
      .model = model.ptr,
    }
  );

  auto ground_geom = dCreatePlane(space, 0, 0, 1, 0);

  for (size_t i = 0; i < 20; i++) {
    for (size_t j = 0; j < 25; j++) {
      auto body = dBodyCreate(world);
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

      auto body_ix = engine::system::ode::register_body(
        session->ode,
        cmp_ode_body.ptr,
        body
      );

      auto transform_ix = lib::flat32::add(cmp_base_transform.ptr);
      // Don't init the value.

      engine::system::entities::added_component_t components[] = {
        { engine::component::type_t::base_transform, transform_ix },
        { engine::component::type_t::ode_body, body_ix },
        { engine::component::type_t::artline_model, model_ix },
      };
      engine::system::entities::add(
        entities.ptr,
        components,
        sizeof(components) / sizeof(*components)
      );
    }
  }
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
    task(
      update_stuff,
      &session->components.artline_model,
      &session->components.base_transform,
      &session->components.ode_body,
      session->entities,
      result.model,
      session
    );
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
