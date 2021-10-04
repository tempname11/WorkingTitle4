#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::secondary_geometry {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkDescriptorSetLayout descriptor_set_layout_textures;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkSampler sampler_albedo;
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace
