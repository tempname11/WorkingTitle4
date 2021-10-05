#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::probe_depth_update {

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
  vkCmdDispatch(cmd,
    PROBE_GRID_SIZE.x,
    PROBE_GRID_SIZE.y,
    PROBE_GRID_SIZE.z
  );
}

} // namespace
