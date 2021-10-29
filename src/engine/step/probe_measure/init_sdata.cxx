#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/session/data.hxx>
#include "internal.hxx"
#include "data.hxx"

namespace engine::step::probe_measure {

void init_sdata(
  SData *out,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkDescriptorSetLayout descriptor_set_layout_textures;

  { ZoneScopedN("descriptor_set_layout_frame");
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
      {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = PROBE_CASCADE_COUNT,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 5,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 6,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 7,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 8,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
      {
        .binding = 9,
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
        &descriptor_set_layout_frame
      );
      assert(result == VK_SUCCESS);
    }
  }

  { ZoneScopedN("descriptor_set_layout_textures");
    VkDescriptorSetLayoutBinding layout_bindings[] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = 1024, // @Incomplete :LargeEnough
        .stageFlags = VK_SHADER_STAGE_ALL,
      },
    };

    VkDescriptorBindingFlags flags = (0
      | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
      // | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    );
    VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount = 1,
      .pBindingFlags = &flags,
    };
    VkDescriptorSetLayoutCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &flags_info,
      .bindingCount = sizeof(layout_bindings) / sizeof(*layout_bindings),
      .pBindings = layout_bindings,
    };
    {
      auto result = vkCreateDescriptorSetLayout(
        core->device,
        &create_info,
        core->allocator,
        &descriptor_set_layout_textures
      );
      assert(result == VK_SUCCESS);
    }
  }

  VkPipelineLayout pipeline_layout;
  { ZoneScopedN("pipeline_layout");
    VkDescriptorSetLayout layouts[] = {
      descriptor_set_layout_frame,
      descriptor_set_layout_textures,
      descriptor_set_layout_textures,
    };

    VkPushConstantRange push_constant_ranges[] = {
      {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PerCascade)
      },
    };

    VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = sizeof(layouts) / sizeof(*layouts),
      .pSetLayouts = layouts,
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
        .codeSize = embedded_probe_measure_comp_len,
        .pCode = (const uint32_t*) embedded_probe_measure_comp,
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

  VkSampler sampler_albedo;
  { ZoneScopedN("sampler_albedo");
    VkSamplerCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .anisotropyEnable = true,
      .maxAnisotropy = core->properties.basic.limits.maxSamplerAnisotropy,
      .maxLod = VK_LOD_CLAMP_NONE,
    };
    auto result = vkCreateSampler(
      core->device,
      &create_info,
      core->allocator,
      &sampler_albedo
    );
    assert(result == VK_SUCCESS);
  }

  VkSampler sampler_probe_irradiance;
  { ZoneScopedN("sampler_probe_irradiance");
    VkSamplerCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
    };
    auto result = vkCreateSampler(
      core->device,
      &create_info,
      core->allocator,
      &sampler_probe_irradiance
    );
    assert(result == VK_SUCCESS);
  }

  VkSampler sampler_trivial;
  { ZoneScopedN("sampler_trivial");
    VkSamplerCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    };
    auto result = vkCreateSampler(
      core->device,
      &create_info,
      core->allocator,
      &sampler_trivial
    );
    assert(result == VK_SUCCESS);
  }

  *out = {
    .descriptor_set_layout_frame = descriptor_set_layout_frame,
    .descriptor_set_layout_textures = descriptor_set_layout_textures,
    .pipeline_layout = pipeline_layout,
    .pipeline = pipeline,
    .sampler_albedo = sampler_albedo,
    .sampler_probe_irradiance = sampler_probe_irradiance,
    .sampler_trivial = sampler_trivial,
  };
}

} // namespace
