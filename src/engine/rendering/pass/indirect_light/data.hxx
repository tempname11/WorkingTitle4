#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::indirect_light {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;
};

struct RData {
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace