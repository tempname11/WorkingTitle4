#include <src/global.hxx>
#include <src/embedded.hxx>
#include "lpass.hxx"

void init_session_finalpass(
  SessionData::Vulkan::Finalpass *out,
  SessionData::Vulkan::Core *core
) {
  ZoneScoped;

  VkDescriptorSetLayout descriptor_set_layout;
  { ZoneScopedN("descriptor_set_layout");
    VkDescriptorSetLayoutBinding layout_bindings[] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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

  VkPipeline pipeline;
  { ZoneScopedN("pipeline");
    VkShaderModule module_comp;
    { ZoneScopedN("module_comp");
      VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = embedded_finalpass_comp_len,
        .pCode = (const uint32_t*) embedded_finalpass_comp,
      };
      auto result = vkCreateShaderModule(
        core->device,
        &create_info,
        core->allocator,
        &module_comp
      );
      assert(result == VK_SUCCESS);
    }
    {
      VkComputePipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_COMPUTE_BIT,
          .module = module_comp,
          .pName = "main"
        },
        .layout = pipeline_layout,
      };
      auto result = vkCreateComputePipelines(
        core->device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        core->allocator,
        &pipeline
      );
      assert(result == VK_SUCCESS);
    }
    vkDestroyShaderModule(
      core->device,
      module_comp,
      core->allocator
    );
  }

  VkSampler sampler_lbuffer;
  { ZoneScopedN(".sampler_lbuffer");
    VkSamplerCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    };
    vkCreateSampler(
      core->device,
      &create_info,
      core->allocator,
      &sampler_lbuffer
    );
  }

  *out = {
    .descriptor_set_layout = descriptor_set_layout,
    .pipeline_layout = pipeline_layout,
    .pipeline = pipeline,
    .sampler_lbuffer = sampler_lbuffer,
  };
}

void deinit_session_finalpass(
  SessionData::Vulkan::Finalpass *it,
  SessionData::Vulkan::Core *core
) {
  vkDestroyPipeline(
    core->device,
    it->pipeline,
    core->allocator
  );
  vkDestroyPipelineLayout(
    core->device,
    it->pipeline_layout,
    core->allocator
  );
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout,
    core->allocator
  );
  vkDestroySampler(
    core->device,
    it->sampler_lbuffer,
    core->allocator
  );
};

void init_rendering_finalpass(
  RenderingData::Finalpass *out,
  RenderingData::Common *common,
  RenderingData::SwapchainDescription *swapchain_description,
  RenderingData::LBuffer *lbuffer,
  RenderingData::FinalImage *final_image,
  SessionData::Vulkan::Finalpass *s_finalpass,
  SessionData::Vulkan::Core *core
) {
  ZoneScoped;
  std::vector<VkDescriptorSet> descriptor_sets;
  descriptor_sets.resize(swapchain_description->image_count);
  std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
  for (auto &layout : layouts) {
    layout = s_finalpass->descriptor_set_layout;
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
  {
    for (size_t i = 0; i < swapchain_description->image_count; i++) {
      VkDescriptorImageInfo final_image_info = {
        .imageView = final_image->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkDescriptorImageInfo lbuffer_image_info = {
        .sampler = s_finalpass->sampler_lbuffer,
        .imageView = lbuffer->views[i],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets[i],
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &final_image_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptor_sets[i],
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &lbuffer_image_info,
        }
      };
      vkUpdateDescriptorSets(
        core->device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
  }

  *out = {
    .descriptor_sets = descriptor_sets,
  };
}

void deinit_rendering_finalpass(
  RenderingData::Finalpass *it,
  SessionData::Vulkan::Core *core
) {
 // empty!
}