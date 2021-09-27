#include <src/lib/gfx/utilities.hxx>
#include <src/engine/common/ubo.hxx>
#include <src/engine/rendering/intra/secondary_gbuffer/constants.hxx>
#include <src/engine/rendering/intra/probe_light_map/constants.hxx>
#include "frame_prepare_uniforms.hxx"

TASK_DECL {
  // @Cleanup: get rid of this task and do this stuff
  // where it actually makes sense to do it.

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
      .probe_info = {
        .grid_size = glm::uvec3(32, 32, 8),
        .grid_world_position_zero = glm::vec3(0.5),
        .grid_world_position_delta = glm::vec3(1.0),
        .light_map_texel_size = glm::vec2(
          engine::rendering::intra::probe_light_map::WIDTH,
          engine::rendering::intra::probe_light_map::HEIGHT
        ),
        .secondary_gbuffer_texel_size = glm::vec2(
          engine::rendering::intra::secondary_gbuffer::WIDTH,
          engine::rendering::intra::secondary_gbuffer::HEIGHT
        ),
      },
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

  // This should not be here! :ManyLights 
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
