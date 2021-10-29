#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::step::probe_measure {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkDescriptorSetLayout descriptor_set_layout_textures;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkSampler sampler_albedo;
  VkSampler sampler_probe_irradiance;
  VkSampler sampler_trivial;
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace
