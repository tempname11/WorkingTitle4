#include <src/lib/gfx/mesh.hxx>
#include "gpass.hxx"

void init_session_gpass(
  SessionData::Vulkan::GPass *out,
  SessionData::Vulkan::Core *core,
  VkShaderModule module_vert,
  VkShaderModule module_frag
) {
  ZoneScoped;
  VkDescriptorSetLayout descriptor_set_layout;
  { ZoneScopedN("descriptor_set_layout");
    VkDescriptorSetLayoutBinding layout_bindings[] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 1,
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
        &descriptor_set_layout
      );
      assert(result == VK_SUCCESS);
    }
  }
  VkPipelineLayout pipeline_layout;
  { ZoneScopedN("pipeline_layout");
    VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout,
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
        .format = GBUFFER_CHANNEL0_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .format = GBUFFER_CHANNEL1_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .format = GBUFFER_CHANNEL2_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .format = ZBUFFER_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
    };
    VkAttachmentReference color_attachment_refs[] = {
      {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
    };
    VkAttachmentReference depth_attachment_ref = {
      .attachment = 3,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass_description = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = sizeof(color_attachment_refs) / sizeof(*color_attachment_refs),
      .pColorAttachments = color_attachment_refs,
      .pDepthStencilAttachment = &depth_attachment_ref,
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
  VkPipeline pipeline;
  { ZoneScopedN("pipeline");
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
        .stride = sizeof(mesh::VertexT05),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
    };
    VkVertexInputAttributeDescription attribute_descriptions[] = {
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(mesh::VertexT05, position),
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(mesh::VertexT05, normal),
      }
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
    VkViewport viewport = { // overridden by dynamic
      .x = 0.0f,
      .y = 0.0f,
      .width = 1.0f,
      .height = 1.0f,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    VkRect2D scissor = { // overridden by dynamic
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
      .cullMode = VK_CULL_MODE_BACK_BIT,
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
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_EQUAL,
    };
    VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
      {
        .blendEnable = VK_FALSE,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
        ),
      },
      {
        .blendEnable = VK_FALSE,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
        ),
      },
      {
        .blendEnable = VK_FALSE,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
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
        &pipeline
      );
      assert(result == VK_SUCCESS);
    }
  }
  *out = {
    .descriptor_set_layout = descriptor_set_layout,
    .pipeline_layout = pipeline_layout,
    .render_pass = render_pass,
    .pipeline = pipeline,
  };
}

void deinit_session_gpass(
  SessionData::Vulkan::GPass *it,
  SessionData::Vulkan::Core *core
) {
  ZoneScoped;
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout,
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
    it->pipeline,
    core->allocator
  );
}

void claim_rendering_gpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  RenderingData::GPass::Stakes *out
) {
  ZoneScoped;
  *out = {};
  out->ubo_material.resize(swapchain_image_count);
  for (auto &stake : out->ubo_material) {
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = (
            sizeof(rendering::UBO_Material)
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

void init_rendering_gpass(
  RenderingData::GPass *out,
  RenderingData::Common *common,
  RenderingData::GPass::Stakes stakes,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::GBuffer *gbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::GPass *s_gpass,
  SessionData::Vulkan::Core *core
) {
  ZoneScoped;
  std::vector<VkDescriptorSet> descriptor_sets;
  { ZoneScopedN("descriptor_sets");
    descriptor_sets.resize(swapchain_description->image_count);
    std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
    for (auto &layout : layouts) {
      layout = s_gpass->descriptor_set_layout;
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
        descriptor_sets.data()
      );
      assert(result == VK_SUCCESS);
    }
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkDescriptorBufferInfo ubo_frame_info = {
        .buffer = common->stakes.ubo_frame[i].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkDescriptorBufferInfo ubo_material_info = {
        .buffer = stakes.ubo_material[i].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets[i],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &ubo_frame_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &ubo_material_info,
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
        gbuffer->channel0_views[i],
        gbuffer->channel1_views[i],
        gbuffer->channel2_views[i],
        zbuffer->views[i],
      };
      VkFramebuffer framebuffer;
      VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = s_gpass->render_pass,
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
    .framebuffers = framebuffers,
    .descriptor_sets = descriptor_sets,
  };
}

void deinit_rendering_gpass(
  RenderingData::GPass *it,
  SessionData::Vulkan::Core *core
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