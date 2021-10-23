#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/display/data.hxx>
#include "internal.hxx"
#include "data.hxx"

namespace engine::step::probe_collect {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sdata->pipeline);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets[frame_info->inflight_index],
    0, nullptr
  );
  for (uint32_t c = 0; c < engine::PROBE_CASCADE_COUNT; c++) {
    PerCascade per_cascade_data = {
      .world_position_delta = engine::PROBE_WORLD_DELTA_C0 * powf(2.0f, c),
      .level = c,
    };
    vkCmdPushConstants(
      cmd,
      sdata->pipeline_layout,
      VK_SHADER_STAGE_COMPUTE_BIT,
      0,
      sizeof(PerCascade),
      &per_cascade_data
    );
    vkCmdDispatch(cmd,
      PROBE_GRID_SIZE.x,
      PROBE_GRID_SIZE.y,
      PROBE_GRID_SIZE.z
    );
  }
}

} // namespace
