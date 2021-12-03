#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
#include "../session/data/vulkan.hxx"

namespace engine::rendering::gpass {
  struct VertexPushConstants {
    glm::mat4 transform;
  };
}

void init_session_gpass(
  engine::session::VulkanData::GPass *out,
  engine::session::VulkanData::Core *core,
  VkShaderModule module_vert,
  VkShaderModule module_frag
);

void deinit_session_gpass(
  engine::session::VulkanData::GPass *it,
  engine::session::VulkanData::Core *core
);

void claim_rendering_gpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  engine::display::Data::GPass::Stakes *out
);

void init_rendering_gpass(
  engine::display::Data::GPass *out,
  engine::display::Data::Helpers *helpers,
  engine::display::Data::GPass::Stakes stakes,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer,
  engine::display::Data::SwapchainDescription *swapchain_description,
  engine::session::VulkanData::GPass *s_gpass,
  engine::session::VulkanData::Core *core
);

void deinit_rendering_gpass(
  engine::display::Data::GPass *it,
  engine::session::VulkanData::Core *core
);