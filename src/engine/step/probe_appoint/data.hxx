#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::step::probe_appoint {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkDescriptorSetLayout descriptor_set_layout_cascade;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

struct DData {
  std::vector<VkDescriptorSet> descriptor_sets_frame;
  std::vector<VkDescriptorSet> descriptor_sets_cascade;
};

} // namespace
