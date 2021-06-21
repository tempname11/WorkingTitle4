#include <src/lib/gfx/utilities.hxx>
#include <src/engine/example.hxx>
#include "task.hxx"

void rendering_frame_example_prepare_uniforms(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<SessionData::State> session_state, // too broad
  usage::Full<RenderingData::Example> example_r
) {
  ZoneScoped;
  auto projection = lib::gfx::utilities::get_projection(
    float(swapchain_description->image_extent.width)
      / swapchain_description->image_extent.height
  );
  auto light_position = glm::vec3(3.0f, 5.0f, 2.0f);
  auto light_intensity = glm::vec3(1000.0f, 1000.0f, 1000.0f);
  auto albedo = glm::vec3(1.0, 1.0, 1.0);
  const example::VS_UBO vs = {
    .projection = projection,
    .view = lib::debug_camera::to_view_matrix(&session_state->debug_camera),
  };
  const example::FS_UBO fs = {
    .camera_position = session_state->debug_camera.position,
    .light_position = light_position,
    .light_intensity = light_intensity,
    .albedo = glm::vec3(1.0f, 1.0f, 1.0f),
    .metallic = 1.0f,
    .roughness = 0.5f,
    .ao = 0.1f,
  };

  void * data;
  vkMapMemory(
    core->device,
    example_r->uniform_stake.memory,
    example_r->uniform_stake.offset
      + frame_info->inflight_index * example_r->total_ubo_aligned_size,
    example_r->total_ubo_aligned_size,
    0, &data
  );
  memcpy(data, &vs, sizeof(example::VS_UBO));
  memcpy((uint8_t*) data + example_r->vs_ubo_aligned_size, &fs, sizeof(example::FS_UBO));
  vkUnmapMemory(core->device, example_r->uniform_stake.memory);
}
