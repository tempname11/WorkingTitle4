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
    const engine::common::ubo::DirectionalLight data = {
      .direction = glm::vec3(0.0f, 0.0f, -1.0f),
      .intensity = 3.0f * glm::vec3(1.0f, 1.0f, 1.0f),
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
