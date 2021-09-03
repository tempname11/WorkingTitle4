#include <src/lib/gfx/utilities.hxx>
#include <src/engine/common/ubo.hxx>
#include "frame_prepare_uniforms.hxx"

TASK_DECL {
  // @Note: this is probably the wrong place to update the buffers!
  // Need to think on what's right.

  ZoneScoped;
  { ZoneScopedN("frame");
    auto projection = lib::gfx::utilities::get_projection(
      float(swapchain_description->image_extent.width)
        / swapchain_description->image_extent.height
    );
    auto view = lib::debug_camera::to_view_matrix(&session_state->debug_camera);
    const engine::common::ubo::Frame data = {
      .projection = projection,
      .view = view,
      .projection_inverse = glm::inverse(projection),
      .view_inverse = glm::inverse(view),
      .flags = session_state->ubo_flags,
      .end_marker = 0xDeadBeef
    };
    auto stake = &common->stakes.ubo_frame[frame_info->inflight_index];
    void * dst;
    vkMapMemory(
      core->device,
      stake->memory,
      stake->offset,
      stake->size,
      0, &dst
    );
    memcpy(dst, &data, sizeof(data));
    vkUnmapMemory(core->device, stake->memory);
  }

  { ZoneScopedN("directional_light");
    auto dir = glm::length(session_state->sun_position_xy) > 1.0f
      ? glm::normalize(session_state->sun_position_xy)
      : session_state->sun_position_xy;
    auto direction = glm::vec3(
      -dir.x,
      -dir.y,
      -std::sqrt(
        std::max(
          0.0f,
          (1.0f
            - (dir.x * dir.x)
            - (dir.y * dir.y)
          )
        )
      )
    );
    const engine::common::ubo::DirectionalLight data = {
      .direction = direction,
      .intensity = session_state->sun_intensity * glm::vec3(1.0f),
    };
    auto stake = &lpass->stakes.ubo_directional_light[frame_info->inflight_index];
    void * dst;
    vkMapMemory(
      core->device,
      stake->memory,
      stake->offset,
      stake->size,
      0, &dst
    );
    memcpy(dst, &data, sizeof(data));
    vkUnmapMemory(core->device, stake->memory);
  }
}
