#include <src/lib/gfx/utilities.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/common/ubo.hxx>
#include <src/engine/rendering/intra/probe_light_map/constants.hxx>
#include <src/engine/rendering/intra/probe_depth_map/constants.hxx>
#include "frame_prepare_uniforms.hxx"

TASK_DECL {
  // @Cleanup: get rid of this task and do this stuff
  // where it actually makes sense to do it.

  ZoneScoped;

  auto dir = glm::length(session_state->sun_position_xy) > 1.0f
    ? glm::normalize(session_state->sun_position_xy)
    : session_state->sun_position_xy;
  auto sun_direction = glm::vec3(
    dir.x,
    dir.y,
    std::sqrt(
      std::max(
        0.0f,
        (1.0f
          - (dir.x * dir.x)
          - (dir.y * dir.y)
        )
      )
    )
  );
  auto sun_intensity = session_state->sun_intensity * glm::vec3(1.0f);

  { ZoneScopedN("frame");
    auto projection = lib::gfx::utilities::get_projection(
      float(swapchain_description->image_extent.width)
        / swapchain_description->image_extent.height
    );
    auto view = lib::debug_camera::to_view_matrix(&session_state->debug_camera);

    auto some_world_offset = glm::vec3(0.5);
    // @Hack: make sure probes are not in the wall for voxel stuff

    auto grid_world_position_zero = (
      glm::floor(
        (session_state->debug_camera.position - some_world_offset)
          / engine::PROBE_WORLD_DELTA
      )
      - 0.5f * glm::vec3(engine::PROBE_GRID_SIZE)
      + 1.0f
    ) * engine::PROBE_WORLD_DELTA + some_world_offset;

    auto grid_world_position_zero_prev = (
      glm::floor(
        (session_state->debug_camera_prev.position - some_world_offset)
          / engine::PROBE_WORLD_DELTA
      )
      - 0.5f * glm::vec3(engine::PROBE_GRID_SIZE)
      + 1.0f
    ) * engine::PROBE_WORLD_DELTA + some_world_offset;

    const engine::common::ubo::Frame data = {
      .projection = projection,
      .view = view,
      .projection_inverse = glm::inverse(projection),
      .view_inverse = glm::inverse(view),
      .sky_sun_direction = sun_direction,
      .sky_intensity = sun_intensity,
      .is_frame_sequential = frame_info->is_sequential,
      .flags = session_state->ubo_flags,
      .probe_info = {
        .random_orientation = lib::gfx::utilities::get_random_rotation(),
        .grid_size = engine::PROBE_GRID_SIZE,
        .grid_size_z_factors = engine::PROBE_GRID_SIZE_Z_FACTORS,
        .change_from_prev = glm::ivec3(
          glm::round( // deal with float division precision
            (grid_world_position_zero - grid_world_position_zero_prev) /
            engine::PROBE_WORLD_DELTA
          )
        ),
        .grid_world_position_zero = grid_world_position_zero,
        .grid_world_position_zero_prev = grid_world_position_zero_prev,
        .grid_world_position_delta = engine::PROBE_WORLD_DELTA,
        .light_map_texel_size = glm::vec2(
          engine::rendering::intra::probe_light_map::WIDTH,
          engine::rendering::intra::probe_light_map::HEIGHT
        ),
        .depth_map_texel_size = glm::vec2(
          engine::rendering::intra::probe_depth_map::WIDTH,
          engine::rendering::intra::probe_depth_map::HEIGHT
        ),
        .secondary_gbuffer_texel_size = glm::vec2(engine::G2_TEXEL_SIZE),
        .depth_sharpness = session_state->probe_depth_sharpness,
        .normal_bias = session_state->probe_normal_bias,
      },
      .end_marker = 0xDeadBeef,
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
    const engine::common::ubo::DirectionalLight data = {
      .direction = -sun_direction, // we have it backwards here
      .intensity = sun_intensity,
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
