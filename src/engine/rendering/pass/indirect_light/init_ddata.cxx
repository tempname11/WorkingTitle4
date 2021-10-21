#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::indirect_light {

void init_ddata(
  DData *out,
  SData *sdata,
  Use<engine::session::Vulkan::Core> core,
  Own<display::Data::Common> common,
  Use<display::Data::GBuffer> gbuffer,
  Use<display::Data::ZBuffer> zbuffer,
  Use<display::Data::LBuffer> lbuffer,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<intra::probe_depth_map::DData> probe_depth_map,
  Use<intra::probe_attention::DData> probe_attention,
  Use<display::Data::SwapchainDescription> swapchain_description
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
      VkDescriptorImageInfo probe_light_map_image_info = {
        .sampler = sdata->sampler_probe_light_map,
        .imageView = probe_light_map->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      #ifdef ENGINE_DEF_ENABLE_PROBE_DEPTH
        VkDescriptorImageInfo probe_depth_map_image_info = {
          .sampler = sdata->sampler_probe_light_map, // @Cleanup
          .imageView = probe_depth_map->views[i],
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
      #endif
      VkDescriptorImageInfo probe_attention_image_info = {
        .imageView = probe_attention->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkDescriptorBufferInfo ubo_frame_info = {
        .buffer = common->stakes.ubo_frame[i].buffer,
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
          .pImageInfo = &probe_light_map_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets_frame[i],
          .dstBinding = 5,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          #ifdef ENGINE_DEF_ENABLE_PROBE_DEPTH
            .pImageInfo = &probe_depth_map_image_info,
          #else
            .pImageInfo = &probe_light_map_image_info,
            // We will never read from it.
          #endif
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
