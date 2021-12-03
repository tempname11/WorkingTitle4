#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/common/ubo.hxx>
#include "../image_formats.hxx"
#include "lpass.hxx"

void init_session_lpass(
  engine::session::VulkanData::LPass *out,
  engine::session::VulkanData::Core *core
) {
  ZoneScoped;

  VkDescriptorSetLayout descriptor_set_layout_frame;
  { ZoneScopedN("descriptor_set_layout_frame");
    VkDescriptorSetLayoutBinding layout_bindings[] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
        .binding = 5,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
    };
    VkDescriptorSetLayoutCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = sizeof(layout_bindings) / sizeof(*layout_bindings),
      .pBindings = layout_bindings,
    };
    {
      auto result = vkCreateDescriptorSetLayout(
        core->device,
        &create_info,
        core->allocator,
        &descriptor_set_layout_frame
      );
      assert(result == VK_SUCCESS);
    }
  }

  VkDescriptorSetLayout descriptor_set_layout_directional_light;
  { ZoneScopedN("descriptor_set_layout_directional_light");
    VkDescriptorSetLayoutBinding layout_bindings[] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
    };
    VkDescriptorSetLayoutCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = sizeof(layout_bindings) / sizeof(*layout_bindings),
      .pBindings = layout_bindings,
    };
    {
      auto result = vkCreateDescriptorSetLayout(
        core->device,
        &create_info,
        core->allocator,
        &descriptor_set_layout_directional_light
      );
      assert(result == VK_SUCCESS);
    }
  }

  VkPipelineLayout pipeline_layout;
  { ZoneScopedN("pipeline_layout");
    VkDescriptorSetLayout layouts[] = {
      descriptor_set_layout_frame,
      descriptor_set_layout_directional_light,
    };
    VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = sizeof(layouts) / sizeof(*layouts),
      .pSetLayouts = layouts,
    };
    {
      auto result = vkCreatePipelineLayout(
        core->device,
        &info,
        core->allocator,
        &pipeline_layout
      );
      assert(result == VK_SUCCESS);
    }
  }

  VkRenderPass render_pass;
  { ZoneScopedN("render_pass");
    VkAttachmentDescription attachment_descriptions[] = {
      {
        .format = LBUFFER2_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,

        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        // This relies on directional_light currently being
        // the first thing that writes to lbuffer. The plan is to do
        // "indirect light" first, then we'll use OP_LOAD here. :IndirectFirst

        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .format = GBUFFER_CHANNEL0_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .format = GBUFFER_CHANNEL1_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .format = GBUFFER_CHANNEL2_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .format = ZBUFFER_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
    };
    VkAttachmentReference color_attachment_refs[] = {
      {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
    };
    VkAttachmentReference input_attachment_refs[] = {
      {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .attachment = 3,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
        .attachment = 4,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
    };
    VkSubpassDescription subpass_description = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = sizeof(input_attachment_refs) / sizeof(*input_attachment_refs),
      .pInputAttachments = input_attachment_refs,
      .colorAttachmentCount = sizeof(color_attachment_refs) / sizeof(*color_attachment_refs),
      .pColorAttachments = color_attachment_refs,
    };
    VkRenderPassCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = sizeof(attachment_descriptions) / sizeof(*attachment_descriptions),
      .pAttachments = attachment_descriptions,
      .subpassCount = 1,
      .pSubpasses = &subpass_description,
    };
    auto result = vkCreateRenderPass(
      core->device,
      &create_info,
      core->allocator,
      &render_pass
    );
    assert(result == VK_SUCCESS);
  }

  VkPipeline pipeline_sun;
  { ZoneScopedN("pipeline_sun");
    VkShaderModule module_frag = VK_NULL_HANDLE;
    VkShaderModule module_vert = VK_NULL_HANDLE;

    { ZoneScopedN("module_vert");
      VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = embedded_lpass_sun_vert_len,
        .pCode = (const uint32_t*) embedded_lpass_sun_vert,
      };
      auto result = vkCreateShaderModule(
        core->device,
        &info,
        core->allocator,
        &module_vert
      );
      assert(result == VK_SUCCESS);
    }

    { ZoneScopedN("module_frag");
      VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = embedded_lpass_sun_frag_len,
        .pCode = (const uint32_t*) embedded_lpass_sun_frag,
      };
      auto result = vkCreateShaderModule(
        core->device,
        &info,
        core->allocator,
        &module_frag
      );
      assert(result == VK_SUCCESS);
    }

    VkPipelineShaderStageCreateInfo shader_stages[] = {
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = module_vert,
        .pName = "main",
      },
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = module_frag,
        .pName = "main",
      },
    };
    VkVertexInputBindingDescription binding_descriptions[] = {
      {
        .binding = 0,
        .stride = sizeof(glm::vec2),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
    };
    VkVertexInputAttributeDescription attribute_descriptions[] = {
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = 0,
      },
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = sizeof(binding_descriptions) / sizeof(*binding_descriptions),
      .pVertexBindingDescriptions = binding_descriptions,
      .vertexAttributeDescriptionCount = sizeof(attribute_descriptions) / sizeof(*attribute_descriptions),
      .pVertexAttributeDescriptions = attribute_descriptions,
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };
    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = 1.0f,
      .height = 1.0f,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    VkRect2D scissor = {
      .offset = {0, 0},
      .extent = {1, 1},
    };
    VkPipelineViewportStateCreateInfo viewport_state_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
    };
    VkPipelineRasterizationStateCreateInfo rasterizer_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo multisample_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
    };
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
    };
    VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
      {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
        ),
      },
    };
    VkPipelineColorBlendStateCreateInfo color_blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = sizeof(color_blend_attachments) / sizeof(*color_blend_attachments),
      .pAttachments = color_blend_attachments,
    };
    VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
    };
    VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pViewportState = &viewport_state_info,
      .pRasterizationState = &rasterizer_info,
      .pMultisampleState = &multisample_info,
      .pDepthStencilState = &depth_stencil_info,
      .pColorBlendState = &color_blend_info,
      .pDynamicState = &dynamic_info,
      .layout = pipeline_layout,
      .renderPass = render_pass,
      .subpass = 0,
    };
    {
      auto result = vkCreateGraphicsPipelines(
        core->device,
        VK_NULL_HANDLE,
        1, &pipeline_info,
        core->allocator,
        &pipeline_sun
      );
      assert(result == VK_SUCCESS);
    }
    vkDestroyShaderModule(core->device, module_frag, core->allocator);
    vkDestroyShaderModule(core->device, module_vert, core->allocator);
  }

  *out = {
    .descriptor_set_layout_frame = descriptor_set_layout_frame,
    .descriptor_set_layout_directional_light = descriptor_set_layout_directional_light,
    .pipeline_layout = pipeline_layout,
    .render_pass = render_pass,
    .pipeline_sun = pipeline_sun,
  };
}

void deinit_session_lpass(
  engine::session::VulkanData::LPass *it,
  engine::session::VulkanData::Core *core
) {
  ZoneScoped;
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_frame,
    core->allocator
  );
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_directional_light,
    core->allocator
  );
  vkDestroyPipelineLayout(
    core->device,
    it->pipeline_layout,
    core->allocator
  );
  vkDestroyRenderPass(
    core->device,
    it->render_pass,
    core->allocator
  );
  vkDestroyPipeline(
    core->device,
    it->pipeline_sun,
    core->allocator
  );
}

void claim_rendering_lpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::LPass::Stakes *out
) {
  ZoneScoped;
  *out = {};
  out->ubo_directional_light.resize(swapchain_image_count);
  for (auto &stake : out->ubo_directional_light) {
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = (
            sizeof(engine::common::ubo::DirectionalLight)
          ),
          .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
      },
      .memory_property_flags = VkMemoryPropertyFlagBits(0
        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      .p_stake_buffer = &stake,
    });
  }
}

void init_rendering_lpass(
  engine::display::Data::LPass *out,
  engine::display::Data::LPass::Stakes stakes,
  engine::display::Data::Helpers *helpers,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer,
  engine::display::Data::LBuffer *lbuffer,
  engine::session::VulkanData::LPass *s_lpass,
  engine::session::VulkanData::Core *core
) {
  ZoneScoped;
  std::vector<VkDescriptorSet> descriptor_sets_frame;

  { ZoneScopedN("descriptor_sets_frame");
    descriptor_sets_frame.resize(swapchain_description->image_count);
    std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
    for (auto &layout : layouts) {
      layout = s_lpass->descriptor_set_layout_frame;
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
    {
      for (size_t i = 0; i < swapchain_description->image_count; i++) {
        VkDescriptorImageInfo channel0_image_info = {
          .imageView = gbuffer->channel0_views[i],
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo channel1_image_info = {
          .imageView = gbuffer->channel1_views[i],
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo channel2_image_info = {
          .imageView = gbuffer->channel2_views[i],
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo depth_image_info = {
          .imageView = zbuffer->views[i],
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo = &channel0_image_info,
          },
          {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_frame[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo = &channel1_image_info,
          },
          {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_frame[i],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo = &channel2_image_info,
          },
          {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_frame[i],
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo = &depth_image_info,
          },
          {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_frame[i],
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &ubo_frame_info,
          },
        };
        vkUpdateDescriptorSets(
          core->device,
          sizeof(writes) / sizeof(*writes), writes,
          0, nullptr
        );
      }
    }
  }

  std::vector<VkDescriptorSet> descriptor_sets_directional_light;
  { ZoneScopedN("descriptor_sets_directional_light");
    descriptor_sets_directional_light.resize(swapchain_description->image_count);
    std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
    for (auto &layout : layouts) {
      layout = s_lpass->descriptor_set_layout_directional_light;
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
        descriptor_sets_directional_light.data()
      );
      assert(result == VK_SUCCESS);
    }
    {
      for (size_t i = 0; i < swapchain_description->image_count; i++) {
        VkDescriptorBufferInfo buffer_info = {
          .buffer = stakes.ubo_directional_light[i].buffer,
          .range = VK_WHOLE_SIZE,
        };
        VkWriteDescriptorSet writes[] = {
          {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_directional_light[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer_info,
          },
        };
        vkUpdateDescriptorSets(
          core->device,
          sizeof(writes) / sizeof(*writes), writes,
          0, nullptr
        );
      }
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
        .renderPass = s_lpass->render_pass,
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
    .stakes = stakes,
    .descriptor_sets_frame = descriptor_sets_frame,
    .descriptor_sets_directional_light = descriptor_sets_directional_light,
    .framebuffers = framebuffers,
  };
}

void deinit_rendering_lpass(
  engine::display::Data::LPass *it,
  engine::session::VulkanData::Core *core
) {
  ZoneScoped;
  for (auto framebuffer : it->framebuffers) {
    vkDestroyFramebuffer(
      core->device,
      framebuffer,
      core->allocator
    );
  }
}