#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::probe_collect {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkSampler sampler_lbuffer; // @Cleanup: remove in favor of common `sampler_trivial`.
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets;
};

} // namespace
