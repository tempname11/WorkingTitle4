#pragma once
#include <src/engine/session/data.hxx>

namespace engine::frame {

void update_physics_transforms(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<system::entities::Impl> entities,
  Own<component::ode_body::storage_t> cmp_ode_body,
  Own<component::base_transform::storage_t> cmp_base_transform,
  Use<system::ode::Impl> ode
);

} // namespace
