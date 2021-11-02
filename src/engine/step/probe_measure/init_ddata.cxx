#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::step::probe_measure {

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<display::Data::Helpers> helpers,
  engine::display::Data::LPass::Stakes* lpass_stakes,
  Use<datum::probe_radiance::DData> lbuffer,
  Use<datum::probe_irradiance::DData> probe_irradiance,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_offsets::SData> probe_offsets,
  Use<datum::probe_attention::DData> probe_attention,
  Use<datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  std::vector<VkDescriptorSet> descriptor_sets_frame;
  descriptor_sets_frame.resize(swapchain_description->image_count);
  std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
  for (auto &layout : layouts) {
    layout = sdata->descriptor_set_layout_frame;
  }
  {
    VkDescriptorSetAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = helpers->descriptor_pool,
      .descriptorSetCount = swapchain_description->image_count,
      .pSetLayouts = layouts.data(),
    };
    auto result = vkAllocateDescriptorSets(
      core->device,
      &allocate_info,
      descriptor_sets_frame.data()
    );
    assert(result == VK_SUCCESS);
  }

  for (size_t i = 0; i < swapchain_description->image_count; i++) {
    VkDescriptorImageInfo lbuffer_info = {
      .imageView = lbuffer->views[i],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorImageInfo probe_irradiance_info = {
      .sampler = sdata->sampler_probe_irradiance,
      .imageView = probe_irradiance->view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo probe_confidence_info = {
      .sampler = sdata->sampler_trivial,
      .imageView = probe_confidence->view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo probe_offsets_info = {
      .sampler = sdata->sampler_trivial,
      .imageView = probe_offsets->view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo probe_attention_info = {
      .imageView = probe_attention->views[i],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorBufferInfo probe_worksets_info[PROBE_CASCADE_COUNT];
    for (size_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
      probe_worksets_info[c] = {
        .buffer = probe_workset->buffers_workset[c].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
    };
    VkDescriptorImageInfo albedo_sampler_info = {
      .sampler = sdata->sampler_albedo,
    };
    VkDescriptorBufferInfo ubo_frame_info = {
      .buffer = helpers->stakes.ubo_frame[i].buffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
    };
    VkDescriptorBufferInfo directional_light_info = {
      .buffer = lpass_stakes->ubo_directional_light[i].buffer,
      .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &lbuffer_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &probe_irradiance_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &probe_confidence_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 3,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &probe_attention_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 4,
        .dstArrayElement = 0,
        .descriptorCount = PROBE_CASCADE_COUNT,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = probe_worksets_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 7,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &ubo_frame_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 8,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &albedo_sampler_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 9,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &directional_light_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_frame[i],
        .dstBinding = 10,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &probe_offsets_info,
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes), writes,
      0, nullptr
    );
  }

  *out = {
    .descriptor_sets_frame = std::move(descriptor_sets_frame),
  };
}

} // namespace
