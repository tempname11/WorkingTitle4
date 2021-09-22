#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::directional_light_secondary {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkDescriptorSetLayout descriptor_set_layout_light;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;
};

struct DData {
  std::vector<VkFramebuffer> framebuffers;
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace