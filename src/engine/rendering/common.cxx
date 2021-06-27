#include <src/lib/gfx/mesh.hxx>
#include "gpass.hxx"

void claim_rendering_common(
  size_t swapchain_image_count,
  std::vector<lib::gfx::multi_alloc::Claim> &claims,
  RenderingData::Common::Stakes *out
) {
  ZoneScoped;
  *out = {};
  out->ubo_frame.resize(swapchain_image_count);
  for (auto &stake : out->ubo_frame) {
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = (
            sizeof(rendering::UBO_Frame)
          ),
          .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
      },
      .memory_property_flags = VkMemoryPropertyFlagBits(0
        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      .p_stake_buffer = &stake,
    });
  }
}

void init_rendering_common(
  RenderingData::Common::Stakes stakes,
  RenderingData::Common *it
) {
  *it = {
    .stakes = stakes,
  };
}