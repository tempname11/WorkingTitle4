#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::step::indirect_light {

void init_ddata(
  DData *out,
  SData *sdata,
  Ref<engine::session::Vulkan::Core> core,
  Own<display::Data::Helpers> helpers,
  Use<display::Data::GBuffer> gbuffer,
  Use<display::Data::ZBuffer> zbuffer,
  Use<display::Data::LBuffer> lbuffer,
  Use<datum::probe_irradiance::DData> probe_irradiance,
  Use<datum::probe_confidence::SData> probe_confidence,
  Use<datum::probe_offsets::SData> probe_offsets,
  Use<datum::probe_attention::DData> probe_attention,
  Ref<display::Data::SwapchainDescription> swapchain_description
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
      .descriptorPool = helpers->descriptor_pool,
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
      VkDescriptorImageInfo gbuffer_channel0_image_info = {
        .imageView = gbuffer->channel0_views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkDescriptorImageInfo gbuffer_channel1_image_info = {
        .imageView = gbuffer->channel1_views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkDescriptorImageInfo gbuffer_channel2_image_info = {
        .imageView = gbuffer->channel2_views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkDescriptorImageInfo zbuffer_image_info = {
        .imageView = zbuffer->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
      VkDescriptorImageInfo probe_attention_image_info = {
        .imageView = probe_attention->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkDescriptorBufferInfo ubo_frame_info = {
        .buffer = helpers->stakes.ubo_frame[i].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &gbuffer_channel0_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &gbuffer_channel1_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 2,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &gbuffer_channel2_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 3,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &zbuffer_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 4,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &probe_irradiance_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 5,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &probe_confidence_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 6,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &ubo_frame_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 7,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &probe_attention_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 8,
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
  }

  std::vector<VkFramebuffer> framebuffers;
  { ZoneScopedN("framebuffers");
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkImageView attachments[] = {
        lbuffer->views[i],
        gbuffer->channel0_views[i],
        gbuffer->channel1_views[i],
        gbuffer->channel2_views[i],
        zbuffer->views[i],
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
