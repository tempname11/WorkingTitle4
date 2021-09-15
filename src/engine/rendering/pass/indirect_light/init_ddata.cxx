#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::indirect_light {

void init_ddata(
  DData *out,
  SData *sdata,
  Use<SessionData::Vulkan::Core> core,
  Own<display::Data::Common> common,
  Use<display::Data::LBuffer> lbuffer,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<engine::display::Data::SwapchainDescription> swapchain_description
) {
  ZoneScoped;

  std::vector<VkDescriptorSet> descriptor_sets_frame;
  { ZoneScopedN("descriptor_sets_frame");
    descriptor_sets_frame.resize(swapchain_description->image_count);

    std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
    for (auto &layout : layouts) {
      layout = sdata->descriptor_set_layout_frame;
    }

    VkDescriptorSetAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = common->descriptor_pool,
      .descriptorSetCount = swapchain_description->image_count,
      .pSetLayouts = layouts.data(),
    };

    {
      auto result = vkAllocateDescriptorSets(
        core->device,
        &allocate_info,
        descriptor_sets_frame.data()
      );
      assert(result == VK_SUCCESS);
    }

    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkDescriptorImageInfo probe_light_map_image_info = {
        .sampler = sdata->sampler_probe_light_map,
        .imageView = probe_light_map->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &probe_light_map_image_info,
        }
      };
      vkUpdateDescriptorSets(
        core->device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
  }

  std::vector<VkFramebuffer> framebuffers;
  { ZoneScopedN("framebuffers");
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkImageView attachments[] = {
        lbuffer->views[i],
      };
      VkFramebuffer framebuffer;
      VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = sdata->render_pass,
        .attachmentCount = sizeof(attachments) / sizeof(*attachments),
        .pAttachments = attachments,
        .width = swapchain_description->image_extent.width,
        .height = swapchain_description->image_extent.height,
        .layers = 1,
      };
      {
        auto result = vkCreateFramebuffer(
          core->device,
          &create_info,
          core->allocator,
          &framebuffer
        );
        assert(result == VK_SUCCESS);
      }
      framebuffers.push_back(framebuffer);
    }
  }

  *out = {
    .framebuffers = std::move(framebuffers),
    .descriptor_sets_frame = std::move(descriptor_sets_frame),
  };
}

} // namespace
