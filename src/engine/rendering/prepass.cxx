#include <src/lib/gfx/mesh.hxx>
#include "prepass.hxx"

void init_example_prepass(
  RenderingData::Example::Prepass *it,
  VkShaderModule module_vert,
  RenderingData::Example::ZBuffer *zbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::Example::GPass *s_gpass,
  SessionData::Vulkan::Core *core
) {
  ZoneScoped;
  { ZoneScopedN(".render_pass");
    VkAttachmentDescription attachment_descriptions[] = {
      {
        .format = ZBUFFER_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
    };
    VkAttachmentReference depth_attachment_ref = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass_description = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
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
      &it->render_pass
    );
    assert(result == VK_SUCCESS);
  }
  { ZoneScopedN(".pipeline");
    VkPipelineShaderStageCreateInfo shader_stages[] = {
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = module_vert,
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
    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = float(swapchain_description->image_extent.width),
      .height = float(swapchain_description->image_extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    VkRect2D scissor = {
      .offset = {0, 0},
      .extent = swapchain_description->image_extent,
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
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
    };
    VkPipelineColorBlendStateCreateInfo color_blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 0,
      .pAttachments = nullptr,
    };
    VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = sizeof(shader_stages) / sizeof(*shader_stages),
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pViewportState = &viewport_state_info,
      .pRasterizationState = &rasterizer_info,
      .pMultisampleState = &multisample_info,
      .pDepthStencilState = &depth_stencil_info,
      .pColorBlendState = &color_blend_info,
      .pDynamicState = nullptr,
      .layout = s_gpass->pipeline_layout,
      .renderPass = it->render_pass,
      .subpass = 0,
    };
    {
      auto result = vkCreateGraphicsPipelines(
        core->device,
        VK_NULL_HANDLE,
        1, &pipeline_info,
        core->allocator,
        &it->pipeline
      );
      assert(result == VK_SUCCESS);
    }
  }
  { ZoneScopedN(".framebuffers");
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkImageView attachments[] = {
        zbuffer->views[i],
      };
      VkFramebuffer framebuffer;
      VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = it->render_pass,
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
      it->framebuffers.push_back(framebuffer);
    }
  }
}
