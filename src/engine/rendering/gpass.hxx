#pragma once
#include <vulkan/vulkan.h>
#include "../display/data.hxx"
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
  engine::display::Data::GPass::Stakes *out
);

void init_rendering_gpass(
  engine::display::Data::GPass *out,
  engine::display::Data::Common *common,
  engine::display::Data::GPass::Stakes stakes,
  engine::display::Data::ZBuffer *zbuffer,
  engine::display::Data::GBuffer *gbuffer,
  engine::display::Data::SwapchainDescription *swapchain_description,
  SessionData::Vulkan::GPass *s_gpass,
  SessionData::Vulkan::Core *core
);

void deinit_rendering_gpass(
  engine::display::Data::GPass *it,
  SessionData::Vulkan::Core *core
);