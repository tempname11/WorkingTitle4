#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/misc.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::secondary_geometry {

void record(
  Use<DData> ddata,
  Use<SData> sdata,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<SessionData::Vulkan::Core> core,
  Ref<engine::common::SharedDescriptorPool> descriptor_pool,
  VkBuffer geometry_refs,
  Use<engine::misc::RenderList> render_list,
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
    VkDescriptorBufferInfo geometry_refs_info = {
      .buffer = geometry_refs,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &write_tlas,
        .dstSet = ddata->descriptor_sets_frame[frame_info->inflight_index],
        .dstBinding = 4,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ddata->descriptor_sets_frame[frame_info->inflight_index],
        .dstBinding = 5,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &geometry_refs_info
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes),
      writes,
      0, nullptr
    );
  }

  VkDescriptorSet descriptor_sets_textures[2];
  { ZoneScoped("descriptor_sets_textures");
    uint32_t count = checked_integer_cast<uint32_t>(
      render_list->items.size()
    );

    {
      std::scoped_lock lock(descriptor_pool->mutex);
      uint32_t counts[] = { count, count };
      VkDescriptorSetLayout layouts[] = {
        sdata->descriptor_set_layout_textures,
        sdata->descriptor_set_layout_textures,
      };
      VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = sizeof(counts) / sizeof(*counts),
        .pDescriptorCounts = counts,
      };
      VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &count_info,
        .descriptorPool = descriptor_pool->pool,
        .descriptorSetCount = sizeof(layouts) / sizeof(*layouts),
        .pSetLayouts = layouts,
      };
      auto result = vkAllocateDescriptorSets(
        core->device,
        &allocate_info,
        descriptor_sets_textures
      );
      assert(result == VK_SUCCESS);
    }

    std::vector<VkDescriptorImageInfo> albedo_infos;
    std::vector<VkDescriptorImageInfo> romeao_infos;
    albedo_infos.resize(count);
    romeao_infos.resize(count);
    for (size_t i = 0; i < count; i++) {
      albedo_infos[i] = {
        .imageView = render_list->items[i].texture_albedo_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      romeao_infos[i] = {
        .imageView = render_list->items[i].texture_romeao_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
    }
    if (count > 0) {
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_textures[0],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = count,
          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          .pImageInfo = albedo_infos.data(),
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_textures[1],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = count,
          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          .pImageInfo = romeao_infos.data(),
        },
      };
      vkUpdateDescriptorSets(
        core->device,
        sizeof(writes) / sizeof(*writes),
        writes,
        0, nullptr
      );
    }
  }

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sdata->pipeline);
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    sdata->pipeline_layout,
    0, 1, &ddata->descriptor_sets_frame[frame_info->inflight_index],
    0, nullptr
  );
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    sdata->pipeline_layout,
    1, 2, descriptor_sets_textures,
    0, nullptr
  );
  vkCmdDispatch(cmd,
    PROBE_GRID_SIZE.x,
    PROBE_GRID_SIZE.y,
    PROBE_GRID_SIZE.z
  );
}

} // namespace
