#include <src/lib/gfx/utilities.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/common/ubo.hxx>
#include <src/engine/rendering/intra/probe_light_map/constants.hxx>
#include <src/engine/rendering/intra/probe_depth_map/constants.hxx>
#include "prepare_uniforms.hxx"

namespace engine::frame {

void prepare_uniforms(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::session::Data::State> session_state,
  Own<engine::display::Data::Common> common,
  Own<engine::display::Data::GPass> gpass,
  Own<engine::display::Data::LPass> lpass
) {
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
    auto view = lib::debug_camera::to_view_matrix(&session_state->debug_camera);
    auto view_prev = lib::debug_camera::to_view_matrix(&session_state->debug_camera_prev);

    auto projection_base = lib::gfx::utilities::get_projection(
      float(swapchain_description->image_extent.width)
        / swapchain_description->image_extent.height
    );
    glm::mat4 jitter = glm::translate(
      glm::mat4(1.0),
      session_state->taa_distance * glm::vec3(
        session_state->taa_jitter_offset.x / swapchain_description->image_extent.width,
        session_state->taa_jitter_offset.y / swapchain_description->image_extent.height,
        0.0
      )
    );
    glm::mat4 jitter_prev = glm::translate(
      glm::mat4(1.0),
      session_state->taa_distance * glm::vec3(
        session_state->taa_jitter_offset_prev.x / swapchain_description->image_extent.width,
        session_state->taa_jitter_offset_prev.y / swapchain_description->image_extent.height,
        0.0
      )
    );
    auto projection_prev = jitter_prev * projection_base;
    auto projection = jitter * projection_base;

    auto some_world_offset = glm::vec3(0.5);
    // @Hack: make sure probes are not in the wall for voxel stuff

    engine::common::ubo::ProbeCascade cascades[engine::common::ubo::MAX_CASCADE_LEVELS];
    assert(engine::PROBE_CASCADE_COUNT <= engine::common::ubo::MAX_CASCADE_LEVELS);
    for (size_t c = 0; c < engine::PROBE_CASCADE_COUNT; c++) {
      auto delta = engine::PROBE_WORLD_DELTA_C0 * powf(2.0f, c);
      auto floor_now = glm::floor(
        (session_state->debug_camera.position - some_world_offset)
          / delta
      );
      auto floor_prev = glm::floor(
        (session_state->debug_camera_prev.position - some_world_offset)
          / delta
      );
      cascades[c] = {
        .world_position_zero = (
          floor_now - 0.5f * glm::vec3(engine::PROBE_GRID_SIZE) + 1.0f
        ) * delta + some_world_offset,
        .world_position_zero_prev = (
          floor_prev - 0.5f * glm::vec3(engine::PROBE_GRID_SIZE) + 1.0f
        ) * delta + some_world_offset,
        .change_from_prev = glm::ivec3(
          floor_now - floor_prev
        ),
      };
    };

    const engine::common::ubo::Frame data = {
      .projection = projection,
      .projection_prev = projection_prev,
      .view = view,
      .view_prev = view_prev,
      .projection_inverse = glm::inverse(projection),
      .projection_prev_inverse = glm::inverse(projection_prev),
      .view_inverse = glm::inverse(view),
      .view_prev_inverse = glm::inverse(view_prev),
      .secondary_gbuffer_texel_size = glm::vec2(engine::G2_TEXEL_SIZE),
      .final_image_texel_size = glm::vec2(
        swapchain_description->image_extent.width,
        swapchain_description->image_extent.height
      ),
      .luminance_moving_average = session_state->luminance_moving_average,
      .sky_sun_direction = sun_direction,
      .sky_intensity = sun_intensity,
      .is_frame_sequential = frame_info->is_sequential,
      .flags = session_state->ubo_flags,
      .probe_info = {
        .random_orientation = lib::gfx::utilities::get_random_rotation(),
        .grid_size = engine::PROBE_GRID_SIZE,
        .cascade_count = engine::PROBE_CASCADE_COUNT,
        .grid_size_z_factors = engine::PROBE_GRID_SIZE_Z_FACTORS,
        .cascade_count_factors = engine::PROBE_CASCADE_COUNT_FACTORS,
        // .cascades = cascades, // this is done below
        .grid_world_position_delta_c0 = engine::PROBE_WORLD_DELTA_C0,
        .light_map_texel_size = glm::vec2(
          engine::rendering::intra::probe_light_map::WIDTH,
          engine::rendering::intra::probe_light_map::HEIGHT
        ),
        .depth_map_texel_size = glm::vec2(
          engine::rendering::intra::probe_depth_map::WIDTH,
          engine::rendering::intra::probe_depth_map::HEIGHT
        ),
        .depth_sharpness = session_state->probe_depth_sharpness,
        .normal_bias = session_state->probe_normal_bias,
      },
      .end_marker = 0xDeadBeef,
    };
    for (size_t c = 0; c < engine::PROBE_CASCADE_COUNT; c++) {
      memcpy(
        (void *) &data.probe_info.cascades[c],
        (void *) &cascades[c],
        sizeof(engine::common::ubo::ProbeCascade)
      );
    }
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

} // namespace