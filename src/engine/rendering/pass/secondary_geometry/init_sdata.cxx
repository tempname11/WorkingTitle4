#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/session.hxx>
#include "data.hxx"

namespace engine::rendering::pass::secondary_geometry {

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
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // not used yet
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
        .codeSize = embedded_secondary_geometry_comp_len,
        .pCode = (const uint32_t*) embedded_secondary_geometry_comp,
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

  *out = {
    .descriptor_set_layout = descriptor_set_layout,
    .pipeline_layout = pipeline_layout,
    .pipeline = pipeline,
  };
}

} // namespace
