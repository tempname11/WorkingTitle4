#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::rendering::pass::indirect_light_secondary {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;
  VkSampler sampler_probe_light_map;
};

struct DData {
  std::vector<VkFramebuffer> framebuffers;
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace