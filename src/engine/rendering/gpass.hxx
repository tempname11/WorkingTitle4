#pragma once
#include <vulkan/vulkan.h>
#include "../rendering.hxx"
#include "../session.hxx"

namespace engine::rendering::gpass {
  struct VertexPushConstants {
    glm::mat4 transform;
  };
}

void init_session_gpass(
  SessionData::Vulkan::GPass *out,
  SessionData::Vulkan::Core *core,
  VkShaderModule module_vert,
  VkShaderModule module_frag
);

void deinit_session_gpass(
  SessionData::Vulkan::GPass *it,
  SessionData::Vulkan::Core *core
);

void claim_rendering_gpass(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  RenderingData::GPass::Stakes *out
);

void init_rendering_gpass(
  RenderingData::GPass *out,
  RenderingData::Common *common,
  RenderingData::GPass::Stakes stakes,
  RenderingData::ZBuffer *zbuffer,
  RenderingData::GBuffer *gbuffer,
  RenderingData::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::GPass *s_gpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_gpass(
  RenderingData::GPass *it,
  SessionData::Vulkan::Core *core
);