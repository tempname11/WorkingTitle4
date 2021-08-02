#include <cassert>
#include <src/global.hxx>
#include <Tracy.hpp>
#include "multi_alloc.hxx"
#include "utilities.hxx"

namespace lib::gfx::multi_alloc {

struct IncompleteStake {
  enum {
    IncompleteBuffer,
    IncompleteImage,
  } type;
  StakeBuffer *p_stake_buffer;
  StakeImage *p_stake_image;
  uint32_t memory_type_index;
};

struct StrategyData {
  std::vector<VkDeviceSize> so_far_per_memory_type;
  std::vector<IncompleteStake> incomplete_stakes;
  bool latest_claim_was_linear;
};

StrategyData _internal_begin(
  Instance *it,
  VkPhysicalDeviceMemoryProperties *memory_properties
) {
  return StrategyData {
    .so_far_per_memory_type = std::vector<VkDeviceSize>(
      memory_properties->memoryTypeCount
    ),
    .incomplete_stakes = std::vector<IncompleteStake>(),
    .latest_claim_was_linear = false,
    // Initial value does not matter: first claim will have offset 0,
    // which satisfies bufferImageGranularity anyway.
  };
}

void _internal_on_image(
  Claim *claim,
  StrategyData *data,
  Instance *it,
  VkPhysicalDeviceProperties *properties,
  VkPhysicalDeviceMemoryProperties *memory_properties,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  VkImage image;
  {
    auto result = vkCreateImage(
      device,
      &claim->info.image,
      allocator,
      &image
    );
    assert(result == VK_SUCCESS);
  }
  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, image, &requirements);

  auto index = utilities::find_memory_type_index(
    memory_properties,
    requirements,
    claim->memory_property_flags
  );

  // see https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#glossary-linear-resource 
  bool is_linear = claim->info.image.tiling == VK_IMAGE_TILING_LINEAR;
  // don't support this DRM format, too finicky
  assert(claim->info.image.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);

  auto so_far = &data->so_far_per_memory_type[index];
  *so_far = utilities::aligned_size(*so_far, requirements.alignment);
  if (is_linear != data->latest_claim_was_linear) {
    *so_far = utilities::aligned_size(
      *so_far,
      properties->limits.bufferImageGranularity
    );
  }

  *claim->p_stake_image = {
    .image = image,
    .memory = VK_NULL_HANDLE,
    .offset = *so_far,
    .size = claim->info.buffer.size,
  };
  data->incomplete_stakes.push_back(IncompleteStake {
    .type = IncompleteStake::IncompleteImage,
    .p_stake_image = claim->p_stake_image,
    .memory_type_index = index,
  });

  data->latest_claim_was_linear = is_linear;
  *so_far += requirements.size;
  it->all_images.push_back(image);
}

void _internal_on_buffer(
  Claim *claim,
  StrategyData *data,
  Instance *it,
  VkPhysicalDeviceProperties *properties,
  VkPhysicalDeviceMemoryProperties *memory_properties,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  assert(claim->info.buffer.size > 0);

  VkBuffer buffer;
  {
    auto result = vkCreateBuffer(
      device,
      &claim->info.buffer,
      allocator,
      &buffer
    );
    assert(result == VK_SUCCESS);
  }
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, buffer, &requirements);

  auto index = utilities::find_memory_type_index(
    memory_properties,
    requirements,
    claim->memory_property_flags
  );

  bool is_linear = true; // buffers are always linear!
  auto so_far = &data->so_far_per_memory_type[index];
  *so_far = utilities::aligned_size(*so_far, requirements.alignment);
  if (is_linear != data->latest_claim_was_linear) {
    *so_far = utilities::aligned_size(
      *so_far,
      properties->limits.bufferImageGranularity
    );
  }

  *claim->p_stake_buffer = {
    .buffer = buffer,
    .memory = VK_NULL_HANDLE,
    .offset = *so_far,
    .size = claim->info.buffer.size,
  },
  data->incomplete_stakes.push_back(IncompleteStake {
    .type = IncompleteStake::IncompleteBuffer,
    .p_stake_buffer = claim->p_stake_buffer,
    .memory_type_index = index,
  });

  data->latest_claim_was_linear = is_linear;
  *so_far += requirements.size;
  it->all_buffers.push_back(buffer);
}

const char *tracy_memory_type_names[] = {
  "GPU memory type 0",
  "GPU memory type 1",
  "GPU memory type 2",
  "GPU memory type 3",
  "GPU memory type 4",
  "GPU memory type 5",
  "GPU memory type 6",
  "GPU memory type 7",
  "GPU memory type 8",
  "GPU memory type 9",
};

void _internal_end(
  StrategyData *data,
  Instance *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  std::vector<VkDeviceMemory> allocations(data->so_far_per_memory_type.size());
  for (uint32_t i = 0; i < data->so_far_per_memory_type.size(); i++) {
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (data->so_far_per_memory_type[i] > 0) {
      VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = data->so_far_per_memory_type[i],
        .memoryTypeIndex = i,
      };
      auto result = vkAllocateMemory(device, &alloc_info, allocator, &memory);
      assert(sizeof(tracy_memory_type_names) / sizeof(*tracy_memory_type_names) >= i);
      TracyAllocN((void *) memory, alloc_info.allocationSize, tracy_memory_type_names[i]);
      assert(result == VK_SUCCESS);
    }
    allocations[i] = memory;
    if (memory != VK_NULL_HANDLE) {
      it->all_memories.push_back({ memory, i });
    }
  }
  for (auto &incomplete : data->incomplete_stakes) {
    if (incomplete.type == IncompleteStake::IncompleteBuffer) {
      incomplete.p_stake_buffer->memory = allocations[incomplete.memory_type_index];
      assert(incomplete.p_stake_buffer->memory != VK_NULL_HANDLE);
      {
        auto result = vkBindBufferMemory(
          device,
          incomplete.p_stake_buffer->buffer,
          incomplete.p_stake_buffer->memory,
          incomplete.p_stake_buffer->offset
        );
        assert(result == VK_SUCCESS);
      }
    } else if (incomplete.type == IncompleteStake::IncompleteImage) {
      incomplete.p_stake_image->memory = allocations[incomplete.memory_type_index];
      assert(incomplete.p_stake_image->memory != VK_NULL_HANDLE);
      {
        auto result = vkBindImageMemory(
          device,
          incomplete.p_stake_image->image,
          incomplete.p_stake_image->memory,
          incomplete.p_stake_image->offset
        );
        assert(result == VK_SUCCESS);
      }
    } else {
      assert(false);
    }
  }
}

void init(
  Instance *it,
  std::vector<Claim> &&claims,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkPhysicalDeviceMemoryProperties *memory_properties
) {
  auto data = _internal_begin(it, memory_properties);
  for (auto &claim : claims) {
    switch (claim.info.structure.type) {
      case (VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO): {
        _internal_on_image(
          &claim,
          &data, it,
          properties,
          memory_properties,
          device, allocator
        );
        break;
      }
      case (VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO): {
        _internal_on_buffer(
          &claim,
          &data, it,
          properties,
          memory_properties,
          device, allocator
        );
        break;
      }
      default: {
        assert(false);
      }
    }
  }
  _internal_end(&data, it, device, allocator);
}

void deinit(
  Instance *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  for (auto elem: it->all_buffers) {
    vkDestroyBuffer(device, elem, allocator);
  }

  for (auto elem: it->all_images) {
    vkDestroyImage(device, elem, allocator);
  }

  for (auto elem: it->all_memories) {
    TracyFreeN((void *) elem.first, tracy_memory_type_names[elem.second]);
    vkFreeMemory(device, elem.first, allocator);
  }
}

} // namespace