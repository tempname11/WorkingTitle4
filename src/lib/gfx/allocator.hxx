#pragma once
#include <shared_mutex>
#include <vulkan/vulkan.h>
#include <src/global.hxx>

namespace lib::gfx::allocator {
  using ID = int64_t;
  // positive values for suballocations,
  // negative values for dedicated allocations.
}

namespace lib::gfx {
  struct Allocator {
    uint32_t memory_type_index;
    VkMemoryPropertyFlagBits memory_property_flags;
    size_t size_region;
    size_t min_size_dedicated;
    char const *debug_name;

    enum class AllocationType {
      Gap = 0,
      Buffer,
      Image,
    };

    union AllocationContent {
      VkBuffer buffer;
      VkImage image;
    };

    struct Suballocation {
      lib::gfx::allocator::ID id;
      size_t offset;
      size_t size;
      AllocationType type;
      AllocationContent content;
    };

    struct Region {
      size_t size_occupied;
      size_t total_active_suballocations;
      bool is_latest_suballocation_linear;
      VkDeviceMemory memory;
      void *mem; // only for HOST_VISIBLE
      lib::gfx::allocator::ID first_id;
      std::vector<Suballocation> suballocations;
    };

    struct DedicatedAllocation {
      lib::gfx::allocator::ID id;
      size_t size;
      VkDeviceMemory memory;
      void *mem; // only for HOST_VISIBLE
      AllocationType type;
      AllocationContent content;
    };

    std::shared_mutex rw_mutex;
    size_t total_suballocations;
    std::vector<Region> regions;

    size_t total_dedicated_allocations;
    std::vector<DedicatedAllocation> dedicated_allocations;
  };
}

namespace lib::gfx::allocator {

void init(
  Allocator *it,
  VkPhysicalDeviceMemoryProperties *properties,
  VkMemoryPropertyFlagBits memory_property_flags,
  size_t size_region,
  char const *debug_name
);

void deinit(
  Allocator *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
);

struct Buffer {
  ID id;
  VkBuffer buffer;
};

struct Image {
  ID id;
  VkImage image;
};

Buffer create_buffer(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkBufferCreateInfo *info
);

Image create_image(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkImageCreateInfo *info
);

struct HostMapping {
  void *mem;
  size_t size;
};

HostMapping get_host_mapping(
  Ref<Allocator> it,
  ID id
);

void destroy_buffer(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  Buffer buffer
);

void destroy_image(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  Image image
);

} // namespace