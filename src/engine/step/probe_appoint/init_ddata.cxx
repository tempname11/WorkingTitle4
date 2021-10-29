#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::step::probe_appoint {

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<display::Data::Helpers> helpers,
  Use<datum::probe_attention::DData> probe_attention,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_workset::SData> probe_workset,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  std::vector<VkDescriptorSet> descriptor_sets_frame;
  { ZoneScoped("descriptor_sets_frame");
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
      auto i_prev = (
        (i + swapchain_description->image_count - 1) %
        swapchain_description->image_count
      );
      VkDescriptorBufferInfo ubo_frame_info = {
        .buffer = helpers->stakes.ubo_frame[i].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkDescriptorImageInfo probe_attention_prev_info = {
        .imageView = probe_attention->views[i_prev],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkDescriptorImageInfo probe_confidence_info = {
        .imageView = probe_confidence->view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &ubo_frame_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &probe_attention_prev_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 2,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &probe_confidence_info,
        },
      };
      vkUpdateDescriptorSets(
        core->device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
  }

  std::vector<VkDescriptorSet> descriptor_sets_cascade;
  { ZoneScoped("descriptor_sets_cascade");
    descriptor_sets_cascade.resize(PROBE_CASCADE_COUNT);
    std::vector<VkDescriptorSetLayout> layouts(PROBE_CASCADE_COUNT);
    for (auto &layout : layouts) {
      layout = sdata->descriptor_set_layout_cascade;
    }
    {
      VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = helpers->descriptor_pool,
        .descriptorSetCount = PROBE_CASCADE_COUNT,
        .pSetLayouts = layouts.data(),
      };
      auto result = vkAllocateDescriptorSets(
        core->device,
        &allocate_info,
        descriptor_sets_cascade.data()
      );
      assert(result == VK_SUCCESS);
    }

    for (size_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
      VkDescriptorBufferInfo probe_workset_info = {
        .buffer = probe_workset->buffers_workset[c].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkDescriptorBufferInfo probe_counter_info = {
        .buffer = probe_workset->buffers_counter[c].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_cascade[c],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = &probe_workset_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_cascade[c],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = &probe_counter_info,
        },
      };
      vkUpdateDescriptorSets(
        core->device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
  }

  *out = {
    .descriptor_sets_frame = std::move(descriptor_sets_frame),
    .descriptor_sets_cascade = std::move(descriptor_sets_cascade),
  };
}

} // namespace
