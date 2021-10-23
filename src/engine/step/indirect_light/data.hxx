#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::step::indirect_light {

struct SData {
  VkDescriptorSetLayout descriptor_set_layout_frame;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;
  VkSampler sampler_probe_irradiance;
};

struct DData {
  std::vector<VkFramebuffer> framebuffers;
  std::vector<VkDescriptorSet> descriptor_sets_frame;
};

} // namespace