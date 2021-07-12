#include <src/engine/common/mesh.hxx>
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
  RenderingData::Common *out,
  SessionData::Vulkan::Core *core
) {
  VkDescriptorPool descriptor_pool;
  { ZoneScopedN("descriptor_pool");
    // "large enough"
    const uint32_t COMMON_DESCRIPTOR_COUNT = 1024;
    const uint32_t COMMON_DESCRIPTOR_MAX_SETS = 256;

    VkDescriptorPoolSize sizes[] = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, COMMON_DESCRIPTOR_COUNT },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, COMMON_DESCRIPTOR_COUNT },
    };
    VkDescriptorPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 
      .maxSets = COMMON_DESCRIPTOR_MAX_SETS,
      .poolSizeCount = 1,
      .pPoolSizes = sizes,
    };
    auto result = vkCreateDescriptorPool(
      core->device,
      &create_info,
      core->allocator,
      &descriptor_pool
    );
    assert(result == VK_SUCCESS);
  }

  *out = {
    .descriptor_pool = descriptor_pool,
    .stakes = stakes,
  };
}

void deinit_rendering_common(
  RenderingData::Common *it,
  SessionData::Vulkan::Core *core
) {
  vkDestroyDescriptorPool(
    core->device,
    it->descriptor_pool,
    core->allocator
  );
}