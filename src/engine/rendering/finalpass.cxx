#include "lpass.hxx"

void init_finalpass(
  RenderingData::Finalpass *it,
  VkDescriptorPool common_descriptor_pool,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan *vulkan
) {
  it->descriptor_sets.resize(swapchain_description->image_count);
  std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
  for (auto &layout : layouts) {
    layout = vulkan->finalpass.descriptor_set_layout;
  }
  VkDescriptorSetAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = common_descriptor_pool,
    .descriptorSetCount = swapchain_description->image_count,
    .pSetLayouts = layouts.data(),
  };
  {
    auto result = vkAllocateDescriptorSets(
      vulkan->core.device,
      &allocate_info,
      it->descriptor_sets.data()
    );
    assert(result == VK_SUCCESS);
  }
  {
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkDescriptorImageInfo final_image_info = {
        .imageView = final_image->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkDescriptorImageInfo lbuffer_image_info = {
        .sampler = vulkan->finalpass.sampler_lbuffer,
        .imageView = lbuffer->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = it->descriptor_sets[i],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &final_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = it->descriptor_sets[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &lbuffer_image_info,
        }
      };
      vkUpdateDescriptorSets(
        vulkan->core.device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
  }
}
