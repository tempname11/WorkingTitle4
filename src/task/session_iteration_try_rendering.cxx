#include <backends/imgui_impl_vulkan.h>
#include <src/embedded.hxx>
#include <src/engine/example.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/lib/gfx/mesh.hxx>
#include "task.hxx"

const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

void session_iteration_try_rendering(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Full<SessionData> session
) {
  ZoneScoped;
  VkCommandBuffer cmd_imgui_setup = VK_NULL_HANDLE;
  task::Task* signal_imgui_setup_finished = nullptr;
  auto vulkan = &session->vulkan;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  { // get capabilities
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      vulkan->physical_device,
      vulkan->window_surface,
      &surface_capabilities
    );
    assert(result == VK_SUCCESS);
  }

  if (
    surface_capabilities.currentExtent.width == 0 ||
    surface_capabilities.currentExtent.height == 0
  ) {
    lib::task::signal(ctx->runner, session_iteration_yarn_end.ptr);
    return;
  }
  
  auto rendering = new RenderingData;
  // how many images are in swapchain?
  uint32_t swapchain_image_count;
  { ZoneScopedN(".presentation");
    VkSwapchainCreateInfoKHR swapchain_create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = vulkan->window_surface,
      .minImageCount = surface_capabilities.minImageCount,
      .imageFormat = SWAPCHAIN_FORMAT,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = surface_capabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = surface_capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
    };
    {
      auto result = vkCreateSwapchainKHR(
        vulkan->core.device,
        &swapchain_create_info,
        vulkan->core.allocator,
        &rendering->presentation.swapchain
      );
      assert(result == VK_SUCCESS);
    }
    {
      auto result = vkGetSwapchainImagesKHR(
        session->vulkan.core.device,
        rendering->presentation.swapchain,
        &swapchain_image_count,
        nullptr
      );
      assert(result == VK_SUCCESS);
    }
    rendering->presentation.swapchain_images.resize(swapchain_image_count);
    {
      auto result = vkGetSwapchainImagesKHR(
        session->vulkan.core.device,
        rendering->presentation.swapchain,
        &swapchain_image_count,
        rendering->presentation.swapchain_images.data()
      );
      assert(result == VK_SUCCESS);
    }
    rendering->presentation.image_acquired.resize(swapchain_image_count);
    rendering->presentation.image_rendered.resize(swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
      VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      {
        auto result = vkCreateSemaphore(
          session->vulkan.core.device,
          &info,
          session->vulkan.core.allocator,
          &rendering->presentation.image_acquired[i]
        );
        assert(result == VK_SUCCESS);
      }
      {
        auto result = vkCreateSemaphore(
          session->vulkan.core.device,
          &info,
          session->vulkan.core.allocator,
          &rendering->presentation.image_rendered[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
    rendering->presentation.latest_image_index = uint32_t(-1);
  }
  rendering->presentation_failure_state.failure = false;
  rendering->swapchain_description.image_count = checked_integer_cast<uint8_t>(swapchain_image_count);
  rendering->swapchain_description.image_extent = surface_capabilities.currentExtent;
  rendering->swapchain_description.image_format = SWAPCHAIN_FORMAT;
  rendering->inflight_gpu.signals.resize(swapchain_image_count, nullptr);
  rendering->latest_frame.timestamp_ns = 0;
  rendering->latest_frame.elapsed_ns = 0;
  rendering->latest_frame.number = uint64_t(-1);
  rendering->latest_frame.inflight_index = uint8_t(-1);
  { ZoneScopedN(".command_pools");
    rendering->command_pools = std::vector<CommandPool2>(swapchain_image_count);
    for (size_t i = 0; i < swapchain_image_count; i++) {
      rendering->command_pools[i].pools.resize(session->info.worker_count);
      for (size_t j = 0; j < session->info.worker_count; j++) {
        VkCommandPool pool;
        {
          VkCommandPoolCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = session->vulkan.queue_family_index,
          };
          auto result = vkCreateCommandPool(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &pool
          );
          assert(result == VK_SUCCESS);
        }
        rendering->command_pools[i].pools[j] = pool;
      }
    }
  }
  { ZoneScopedN(".frame_rendered_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->frame_rendered_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  { ZoneScopedN(".example_finished_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->example_finished_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  { ZoneScopedN(".imgui_finished_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->imgui_finished_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  rendering->example.vs_ubo_aligned_size = lib::gfx::utilities::aligned_size(
    sizeof(example::VS_UBO),
    session->vulkan.properties.basic.limits.minUniformBufferOffsetAlignment
  );
  rendering->example.fs_ubo_aligned_size = lib::gfx::utilities::aligned_size(
    sizeof(example::FS_UBO),
    session->vulkan.properties.basic.limits.minUniformBufferOffsetAlignment
  );
  rendering->example.total_ubo_aligned_size = (
    rendering->example.vs_ubo_aligned_size +
    rendering->example.fs_ubo_aligned_size
  );
  { ZoneScopedN(".multi_alloc");
    std::vector<lib::gfx::multi_alloc::Claim> claims;
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = (
            rendering->example.total_ubo_aligned_size
            * rendering->swapchain_description.image_count
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
      .p_stake_buffer = &rendering->example.uniform_stake,
    });
    rendering->example.image_stakes.resize(
      rendering->swapchain_description.image_count
    );
    rendering->example.depth_stakes.resize(
      rendering->swapchain_description.image_count
    );
    for (auto &stake : rendering->example.image_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = rendering->swapchain_description.image_format,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->example.depth_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = DEPTH_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    lib::gfx::multi_alloc::init(
      &rendering->multi_alloc,
      std::move(claims),
      session->vulkan.core.device,
      session->vulkan.core.allocator,
      &session->vulkan.properties.basic,
      &session->vulkan.properties.memory
    );
  }
  { ZoneScopedN(".example");
    { ZoneScopedN("update_descriptor_set");
      VkDescriptorBufferInfo vs_info = {
        .buffer = rendering->example.uniform_stake.buffer,
        .offset = 0,
        .range = sizeof(example::VS_UBO),
      };
      VkDescriptorBufferInfo fs_info = {
        .buffer = rendering->example.uniform_stake.buffer,
        .offset = 0,
        .range = sizeof(example::FS_UBO),
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = session->vulkan.example.descriptor_set,
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          .pBufferInfo = &vs_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = session->vulkan.example.descriptor_set,
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          .pBufferInfo = &fs_info,
        },
      };
      vkUpdateDescriptorSets(
        session->vulkan.core.device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
    { ZoneScopedN(".render_pass");
      VkAttachmentDescription attachment_descriptions[] = {
        {
          .format = rendering->swapchain_description.image_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
          .format = DEPTH_FORMAT,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
      };
      VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription subpass_description = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
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
        vulkan->core.device,
        &create_info,
        vulkan->core.allocator,
        &rendering->example.render_pass
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".pipeline");
      VkShaderModule module_frag = VK_NULL_HANDLE;
      { ZoneScopedN("module_frag");
        VkShaderModuleCreateInfo info = {
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = embedded_example_rs_frag_len,
          .pCode = (const uint32_t*) embedded_example_rs_frag,
        };
        auto result = vkCreateShaderModule(
          vulkan->core.device,
          &info,
          vulkan->core.allocator,
          &module_frag
        );
        assert(result == VK_SUCCESS);
      }
      VkShaderModule module_vert = VK_NULL_HANDLE;
      { ZoneScopedN("module_vert");
        VkShaderModuleCreateInfo info = {
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = embedded_example_rs_vert_len,
          .pCode = (const uint32_t*) embedded_example_rs_vert,
        };
        auto result = vkCreateShaderModule(
          vulkan->core.device,
          &info,
          vulkan->core.allocator,
          &module_vert
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
        .width = float(rendering->swapchain_description.image_extent.width),
        .height = float(rendering->swapchain_description.image_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
      };
      VkRect2D scissor = {
        .offset = {0, 0},
        .extent = rendering->swapchain_description.image_extent,
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
      VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
        ),
      };
      VkPipelineColorBlendStateCreateInfo color_blend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
      };
      VkGraphicsPipelineCreateInfo pipelineInfo = {
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
        .pDynamicState = nullptr,
        .layout = vulkan->example.pipeline_layout,
        .renderPass = rendering->example.render_pass,
        .subpass = 0,
      };
      {
        auto result = vkCreateGraphicsPipelines(
          vulkan->core.device,
          VK_NULL_HANDLE,
          1, &pipelineInfo,
          vulkan->core.allocator,
          &rendering->example.pipeline
        );
        assert(result == VK_SUCCESS);
      }
      vkDestroyShaderModule(vulkan->core.device, module_frag, vulkan->core.allocator);
      vkDestroyShaderModule(vulkan->core.device, module_vert, vulkan->core.allocator);
    }
    { ZoneScopedN(".image_views");
      for (auto stake : rendering->example.image_stakes) {
        VkImageView image_view;
        VkImageViewCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = stake.image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = rendering->swapchain_description.image_format,
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
          },
        };
        {
          auto result = vkCreateImageView(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &image_view
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.image_views.push_back(image_view);
      }
    }
    { ZoneScopedN(".depth_views");
      for (auto stake : rendering->example.depth_stakes) {
        VkImageView depth_view;
        VkImageViewCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = stake.image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = DEPTH_FORMAT,
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1,
          },
        };
        {
          auto result = vkCreateImageView(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &depth_view
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.depth_views.push_back(depth_view);
      }
    }
    { ZoneScopedN(".framebuffers");
      assert(rendering->example.image_views.size() == rendering->example.depth_views.size());
      for (size_t i = 0; i < rendering->example.image_views.size(); i++) {
        VkImageView attachments[] = {
          rendering->example.image_views[i],
          rendering->example.depth_views[i],
        };
        VkFramebuffer framebuffer;
        VkFramebufferCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = rendering->example.render_pass,
          .attachmentCount = sizeof(attachments) / sizeof(*attachments),
          .pAttachments = attachments,
          .width = rendering->swapchain_description.image_extent.width,
          .height = rendering->swapchain_description.image_extent.height,
          .layers = 1,
        };
        {
          auto result = vkCreateFramebuffer(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &framebuffer
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.framebuffers.push_back(framebuffer);
      }
    }
  }

  { ZoneScopedN(".imgui_backend");
    { ZoneScopedN(".setup_command_pool");
      VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = session->vulkan.queue_family_index,
      };
      auto result = vkCreateCommandPool(
        session->vulkan.core.device,
        &create_info,
        session->vulkan.core.allocator,
        &rendering->imgui_backend.setup_command_pool
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".setup_semaphore");
      VkSemaphoreTypeCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      };
      VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_info,
      };
      auto result = vkCreateSemaphore(
        session->vulkan.core.device,
        &create_info,
        session->vulkan.core.allocator,
        &rendering->imgui_backend.setup_semaphore
      );
      assert(result == VK_SUCCESS);
    }
    signal_imgui_setup_finished = lib::gpu_signal::create(
      &session->gpu_signal_support,
      session->vulkan.core.device,
      rendering->imgui_backend.setup_semaphore,
      1
    );
    { ZoneScopedN(".render_pass");
      VkRenderPass render_pass;
      VkAttachmentDescription color_attachment = {
        .format = rendering->swapchain_description.image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      };
      VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
      };
      VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
      };
      {
        auto result = vkCreateRenderPass(
          session->vulkan.core.device,
          &create_info,
          session->vulkan.core.allocator,
          &rendering->imgui_backend.render_pass
        );
        assert(result == VK_SUCCESS);
      }
    }
    { ZoneScopedN(".framebuffers");
      // @Note: we reuse example.image_views here.
      for (size_t i = 0; i < rendering->example.image_views.size(); i++) {
        VkImageView attachments[] = {
          rendering->example.image_views[i],
        };
        VkFramebuffer framebuffer;
        VkFramebufferCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = rendering->imgui_backend.render_pass,
          .attachmentCount = sizeof(attachments) / sizeof(*attachments),
          .pAttachments = attachments,
          .width = rendering->swapchain_description.image_extent.width,
          .height = rendering->swapchain_description.image_extent.height,
          .layers = 1,
        };
        {
          auto result = vkCreateFramebuffer(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &framebuffer
          );
          assert(result == VK_SUCCESS);
        }
        rendering->imgui_backend.framebuffers.push_back(framebuffer);
      }
    }
    { ZoneScopedN("init");
      ImGui_ImplVulkan_InitInfo info = {
        .Instance = session->vulkan.instance,
        .PhysicalDevice = session->vulkan.physical_device,
        .Device = session->vulkan.core.device,
        .QueueFamily = session->vulkan.queue_family_index,
        .Queue = session->vulkan.queue_work,
        .DescriptorPool = session->vulkan.common_descriptor_pool,
        .Subpass = 0,
        .MinImageCount = rendering->swapchain_description.image_count,
        .ImageCount = rendering->swapchain_description.image_count,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = session->vulkan.core.allocator,
      };
      ImGui_ImplVulkan_Init(&info, rendering->imgui_backend.render_pass);
    }
    { ZoneScopedN("fonts_texture");
      auto alloc_info = VkCommandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = rendering->imgui_backend.setup_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };
      vkAllocateCommandBuffers(
        session->vulkan.core.device,
        &alloc_info,
        &cmd_imgui_setup
      );
      { // begin
        VkCommandBufferBeginInfo begin_info = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        auto result = vkBeginCommandBuffer(cmd_imgui_setup, &begin_info);
        assert(result == VK_SUCCESS);
      }
      ImGui_ImplVulkan_CreateFontsTexture(cmd_imgui_setup);
      { // end
        auto result = vkEndCommandBuffer(cmd_imgui_setup);
        assert(result == VK_SUCCESS);
      }
    }
  }
  auto rendering_yarn_end = lib::task::create_yarn_signal();
  auto task_frame = task::create(
    rendering_frame,
    rendering_yarn_end,
    session.ptr,
    rendering,
    &session->glfw,
    &rendering->presentation_failure_state,
    &rendering->latest_frame,
    &rendering->swapchain_description,
    &rendering->inflight_gpu
  );
  auto task_cleanup = task::create(defer, task::create(
    rendering_cleanup,
    session_iteration_yarn_end.ptr,
    session.ptr,
    rendering
  ));
  auto task_imgui_setup_cleanup = task::create(defer, task::create(
    rendering_imgui_setup_cleanup,
    &session->vulkan.core,
    &rendering->imgui_backend
  ));
  task::inject(ctx->runner, {
    task_frame,
    task_imgui_setup_cleanup,
    task_cleanup,
  }, {
    .new_dependencies = {
      { signal_imgui_setup_finished, task_frame },
      { signal_imgui_setup_finished, task_imgui_setup_cleanup },
      { rendering_yarn_end, task_cleanup }
    },
    .changed_parents = { { .ptr = rendering, .children = {
      &rendering->presentation,
      &rendering->presentation_failure_state,
      &rendering->swapchain_description,
      &rendering->inflight_gpu,
      &rendering->imgui_backend,
      &rendering->latest_frame,
      &rendering->command_pools,
      &rendering->example_finished_semaphore,
      &rendering->imgui_finished_semaphore,
      &rendering->frame_rendered_semaphore,
      &rendering->multi_alloc,
      &rendering->example,
    } } },
  });
  { ZoneScopedN("submit_imgui_setup");
    // this needs to happen after `inject`,
    // so that the signal is still valid here.

    // check sanity
    assert(signal_imgui_setup_finished != nullptr);
    assert(cmd_imgui_setup != VK_NULL_HANDLE);

    uint64_t one = 1;
    auto timeline_info = VkTimelineSemaphoreSubmitInfo {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .signalSemaphoreValueCount = 1,
      .pSignalSemaphoreValues = &one,
    };
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_imgui_setup,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &rendering->imgui_backend.setup_semaphore,
    };
    auto result = vkQueueSubmit(
      session->vulkan.queue_work,
      1, &submit_info,
      VK_NULL_HANDLE
    );
    assert(result == VK_SUCCESS);
  }
}
