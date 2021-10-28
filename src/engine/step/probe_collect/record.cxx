#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/datum/probe_workset.hxx>
#include "internal.hxx"
#include "data.hxx"

namespace engine::step::probe_collect {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::datum::probe_workset::SData> probe_workset,
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
    vkCmdDispatchIndirect(
      cmd,
      probe_workset->buffers_counter[c].buffer,
      0
    );
  }
}

} // namespace
