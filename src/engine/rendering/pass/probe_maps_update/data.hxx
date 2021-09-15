#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::probe_maps_update {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets;
};

} // namespace
