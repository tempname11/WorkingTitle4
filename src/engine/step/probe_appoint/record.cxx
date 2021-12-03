#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/misc.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/datum/probe_workset/data.hxx>
#include "internal.hxx"
#include "data.hxx"

namespace engine::step::probe_appoint {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Ref<engine::datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::session::VulkanData::Core> core,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  for (uint32_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
    vkCmdFillBuffer(cmd, probe_workset->buffers_counter[c].buffer, 0, 4, 0);
    vkCmdFillBuffer(cmd, probe_workset->buffers_counter[c].buffer, 4, 8, 1); // @Endian
  }

  {
    std::vector<VkBufferMemoryBarrier> barriers;
    for (uint32_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
      barriers.push_back(VkBufferMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = probe_workset->buffers_counter[c].buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
      });
    }
    vkCmdPipelineBarrier(
      cmd,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      0, nullptr,
      barriers.size(), barriers.data(),
      0, nullptr
    );
  }

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sdata->pipeline);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );

  for (uint32_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
      sdata->pipeline_layout,
      1, 1, &ddata->descriptor_sets_cascade[c],
      0, nullptr
    );
    PerCascade per_cascade_data = { .level = c };
    vkCmdPushConstants(
      cmd,
      sdata->pipeline_layout,
      VK_SHADER_STAGE_COMPUTE_BIT,
      0,
      sizeof(PerCascade),
      &per_cascade_data
    );
    vkCmdDispatch(cmd,
      PROBE_GRID_SIZE.x / 8, // :ProbeAppointLocalSize
      PROBE_GRID_SIZE.y / 8,
      PROBE_GRID_SIZE.z
    );
  }
}

} // namespace
