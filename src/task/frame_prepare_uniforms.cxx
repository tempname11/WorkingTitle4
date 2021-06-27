#include <src/lib/gfx/utilities.hxx>
#include "task.hxx"

void frame_prepare_uniforms(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<SessionData::State> session_state, // too broad
  usage::Full<RenderingData::Common> common,
  usage::Full<RenderingData::GPass> gpass,
  usage::Full<RenderingData::LPass> lpass
) {
  // @Note: this is probably the wrong place to update the buffers!
  // Need to think on what's right.

  ZoneScoped;
  { ZoneScopedN("frame");
    auto projection = lib::gfx::utilities::get_projection(
      float(swapchain_description->image_extent.width)
        / swapchain_description->image_extent.height
    );
    auto view = lib::debug_camera::to_view_matrix(&session_state->debug_camera);
    const rendering::UBO_Frame data = {
      .projection = projection,
      .view = view,
      .projection_inverse = glm::inverse(projection),
      .view_inverse = glm::inverse(view),
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

  { ZoneScopedN("material");
    auto albedo = glm::vec3(1.0, 1.0, 1.0);
    const rendering::UBO_Material data = {
      .albedo = glm::vec3(1.0f, 1.0f, 1.0f),
      .metallic = 0.5f,
      .roughness = 0.5f,
      .ao = 0.1f,
    };
    auto stake = &gpass->stakes.ubo_material[frame_info->inflight_index];
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
    const rendering::UBO_DirectionalLight data = {
      .direction = glm::vec3(0.0f, 0.0f, -1.0f),
      .intensity = 10.0f * glm::vec3(1.0f, 1.0f, 1.0f),
    };
    auto stake = &lpass->ubo_directional_light_stakes[frame_info->inflight_index];
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
