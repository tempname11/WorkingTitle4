#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::probe_measure {

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<display::Data::Common> common,
  engine::display::Data::LPass::Stakes* lpass_stakes,
  Use<intra::secondary_lbuffer::DData> lbuffer,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<intra::probe_attention::DData> probe_attention,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::session::Vulkan::Core> core
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
      .descriptorPool = common->descriptor_pool,
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
    auto i_prev = (
      (i + swapchain_description->image_count - 1) %
      swapchain_description->image_count
    );
    VkDescriptorImageInfo lbuffer_info = {
      .imageView = lbuffer->views[i],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorImageInfo probe_light_map_prev_image_info = {
      .sampler = sdata->sampler_probe_light_map,
      .imageView = probe_light_map->views[i_prev],
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo probe_attention_info = {
      .imageView = probe_attention->views[i],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorImageInfo probe_attention_prev_info = {
      .imageView = probe_attention->views[i_prev],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorImageInfo albedo_sampler_info = {
      .sampler = sdata->sampler_albedo,
    };
    VkDescriptorBufferInfo ubo_frame_info = {
      .buffer = common->stakes.ubo_frame[i].buffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
    };
    VkDescriptorBufferInfo tmp_directional_light_info = {
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
        .pImageInfo = &probe_light_map_prev_image_info,
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
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &probe_attention_prev_info,
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
        .pBufferInfo = &tmp_directional_light_info,
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
