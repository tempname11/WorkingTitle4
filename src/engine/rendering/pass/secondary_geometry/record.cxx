#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::secondary_geometry {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<SessionData::Vulkan::Core> core,
  VkBuffer render_list,
  VkAccelerationStructureKHR accel,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  {
    VkWriteDescriptorSetAccelerationStructureKHR write_tlas = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &accel,
    };
    VkDescriptorBufferInfo render_list_info = {
      .buffer = render_list,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &write_tlas,
        .dstSet = ddata->descriptor_sets[frame_info->inflight_index],
        .dstBinding = 4,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ddata->descriptor_sets[frame_info->inflight_index],
        .dstBinding = 5,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &render_list_info
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes),
      writes,
      0, nullptr
    );
  }

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sdata->pipeline);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets[frame_info->inflight_index],
    0, nullptr
  );
  // @Incomplete: probe grid
  vkCmdDispatch(cmd,
    32, // w
    32, // h
    8 // d
  );
}

} // namespace
