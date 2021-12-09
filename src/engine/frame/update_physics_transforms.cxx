#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/system/entities/impl.hxx>
#include <src/engine/component/base_transform.hxx>
#include <src/engine/component/ode_body.hxx>

namespace engine::frame {

void update_physics_transforms(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<system::entities::Impl> entities,
  Own<component::ode_body::storage_t> cmp_ode_body,
  Own<component::base_transform::storage_t> cmp_base_transform,
  Use<system::ode::Impl> ode
) {
  ZoneScoped;

  for (size_t i = 0; i < entities->types->count; i++) {
    auto ty = &entities->types->data[i];
    
    auto mask = (0
      | (1 << uint64_t(component::type_t::base_transform))
      | (1 << uint64_t(component::type_t::ode_body))
    );

    if ((ty->component_bit_mask & mask) != mask) {
      continue;
    }

    for (size_t j = 0; j < ty->indices->count; j += ty->component_count) {
      auto base_transform = &cmp_base_transform->values->data[
        ty->indices->data[
          j + ty->component_index_offsets[uint8_t(component::type_t::base_transform)]
        ]
      ];

      auto ode_body = &cmp_ode_body->values->data[
        ty->indices->data[
          j + ty->component_index_offsets[uint8_t(component::type_t::ode_body)]
        ]
      ];

      if (ode_body->updated_this_frame) {
        ode_body->updated_this_frame = false;

        auto r = dBodyGetRotation(ode_body->body);
        auto p = dBodyGetPosition(ode_body->body);
        base_transform->matrix = {
          r[0], r[4], r[8], 0,
          r[1], r[5], r[9], 0,
          r[2], r[6], r[10], 0,
          p[0], p[1], p[2], 1,
        };
      }
    }
  }
}

} // namespace
