#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/session.hxx>
#include "internal.hxx"
#include "data.hxx"

namespace engine::rendering::pass::probe_light_update {

void init_sdata(
  SData *out,
  Use<SessionData::Vulkan::Core> core
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
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 3,
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
    VkPushConstantRange push_constant_ranges[] = {
      {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PerCascade)
      },
    };

    VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout,
      .pushConstantRangeCount = sizeof(push_constant_ranges) / sizeof(*push_constant_ranges),
      .pPushConstantRanges = push_constant_ranges,
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
        .codeSize = embedded_probe_light_update_comp_len,
        .pCode = (const uint32_t*) embedded_probe_light_update_comp,
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
  { ZoneScopedN("sampler_lbuffer");
    VkSamplerCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    };
    vkCreateSampler( // @Cleanup check result
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

} // namespace
