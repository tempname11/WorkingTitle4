#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::secondary_geometry {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets;
};

} // namespace
