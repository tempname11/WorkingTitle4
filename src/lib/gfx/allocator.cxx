#include <algorithm>
#include <vector>
#include "utilities.hxx"
#include "allocator.hxx"

namespace lib::gfx::allocator {

const float MIN_SIZE_DEDICATED_FRACTION = 0.25;

void init(
  Allocator *out,
  VkPhysicalDeviceMemoryProperties *properties,
  VkMemoryPropertyFlagBits memory_property_flags,
  size_t size_region,
  char const *debug_name
) {
  ZoneScoped;

  uint32_t memory_type_index = (uint32_t) -1;
  for (uint32_t i = 0; i < properties->memoryTypeCount; i++) {
    if ((properties->memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags) {
      memory_type_index = i;
      break;
    }
  }
  assert(memory_type_index != (uint32_t) -1);

  *out = {
    .memory_type_index = memory_type_index,
    .memory_property_flags = memory_property_flags,
    .size_region = size_region,
    .min_size_dedicated = size_t(size_region * MIN_SIZE_DEDICATED_FRACTION),
    .debug_name = debug_name,
  };
  lib::mutex::init(&out->mutex);
}

void deinit(
  Allocator *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  for (auto &item : it->dedicated_allocations) {
    vkFreeMemory(device, item.memory, allocator);
    TracyFreeN((void *) item.memory, it->debug_name);
    if (item.type == Allocator::AllocationType::Buffer) {
      vkDestroyBuffer(device, item.content.buffer, allocator);
    }
    if (item.type == Allocator::AllocationType::Image) {
      vkDestroyImage(device, item.content.image, allocator);
    }
  }

  for (auto &item : it->regions) {
    vkFreeMemory(device, item.memory, allocator);
    TracyFreeN((void *) item.memory, it->debug_name);
    for (auto &item : item.suballocations) {
      if (item.type == Allocator::AllocationType::Buffer) {
        vkDestroyBuffer(device, item.content.buffer, allocator);
      }
      if (item.type == Allocator::AllocationType::Image) {
        vkDestroyImage(device, item.content.image, allocator);
      }
    }
  }

  lib::mutex::deinit(&it->mutex);
}

struct _Allocation {
  ID id;
  VkDeviceMemory memory;
  size_t offset_start;
};

_Allocation _allocate(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkMemoryRequirements *requirements,
  Allocator::AllocationType type,
  Allocator::AllocationContent content,
  bool is_linear
) {
  assert(requirements->memoryTypeBits & (1 << it->memory_type_index) != 0);

  _Allocation alloc = {};
  lib::mutex::lock(&it->mutex);

  if (requirements->size >= it->min_size_dedicated) {
    assert(it->total_suballocations < INT64_MAX);
    it->total_dedicated_allocations++;

    auto id = ID(-it->total_dedicated_allocations);

    VkDeviceMemory memory;
    {
      VkMemoryAllocateFlagsInfo flags_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
      };
      VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &flags_info,
        .allocationSize = requirements->size,
        .memoryTypeIndex = it->memory_type_index,
      };
      auto result = vkAllocateMemory(device, &alloc_info, allocator, &memory);
      assert(result == VK_SUCCESS);
      TracyAllocN((void *) memory, alloc_info.allocationSize, it->debug_name);
    }

    void *mem = nullptr;
    if ((it->memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
      auto result = vkMapMemory(
        device,
        memory,
        0,
        requirements->size,
        0,
        &mem
      );
      assert(result == VK_SUCCESS);
    }

    it->dedicated_allocations.push_back({
      .id = id,
      .size = requirements->size,
      .memory = memory,
      .mem = mem,
      .type = type,
      .content = content,
    });

    alloc = {
      .id = id,
      .memory = memory,
      .offset_start = 0,
    };
  } else {
    assert(it->total_suballocations < INT64_MAX);
    it->total_suballocations++;

    auto id = ID(it->total_suballocations);

    bool need_new_region = false;
    if (it->regions.size() == 0) {
      need_new_region = true;
    } else {
      auto region = &it->regions[it->regions.size() - 1];
      auto offset_start = utilities::aligned_size(
        region->size_occupied,
        std::max(
          requirements->alignment,
          is_linear != region->is_latest_suballocation_linear
            ? properties->limits.bufferImageGranularity
            : 1
        )
      );
      auto offset_end = offset_start + requirements->size;
      if (offset_end > it->size_region) {
        need_new_region = true;
      } else {
        alloc = {
          .id = id,
          .memory = region->memory,
          .offset_start = offset_start,
        };
      }
    }

    if (need_new_region) {
      VkDeviceMemory memory;
      {
        VkMemoryAllocateFlagsInfo flags_info = {
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
          .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        };
        VkMemoryAllocateInfo alloc_info = {
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          .pNext = &flags_info,
          .allocationSize = it->size_region,
          .memoryTypeIndex = it->memory_type_index,
        };
        auto result = vkAllocateMemory(device, &alloc_info, allocator, &memory);
        assert(result == VK_SUCCESS);
        TracyAllocN((void *) memory, alloc_info.allocationSize, it->debug_name);
      }

      void *mem = nullptr;
      if ((it->memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
        auto result = vkMapMemory(
          device,
          memory,
          0,
          it->size_region,
          0,
          &mem
        );
        assert(result == VK_SUCCESS);
      }

      it->regions.push_back({
        .memory = memory,
        .mem = mem,
        .first_id = id,
      });

      assert(it->total_suballocations <= INT64_MAX);
      alloc = {
        .id = id,
        .memory = memory,
        .offset_start = 0,
      };
    }

    auto region = &it->regions[it->regions.size() - 1];
    // `alloc` is already valid here
    auto offset_end = alloc.offset_start + requirements->size;
    assert(offset_end <= it->size_region);

    region->total_active_suballocations++;
    region->size_occupied = offset_end;
    region->is_latest_suballocation_linear = is_linear;
    region->suballocations.push_back({
      .id = id,
      .offset = alloc.offset_start,
      .size = requirements->size,
      .type = type,
      .content = content,
    });
  }

  lib::mutex::unlock(&it->mutex);
  return alloc;
}

Buffer create_buffer(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkBufferCreateInfo *info
) {
  ZoneScoped;

  VkBuffer buffer;
  {
    auto result = vkCreateBuffer(
      device,
      info,
      allocator,
      &buffer
    );
    assert(result == VK_SUCCESS);
  }
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, buffer, &requirements);
  bool is_linear = true; // buffers are always linear!

  auto alloc = _allocate(
    it,
    device,
    allocator,
    properties,
    &requirements,
    Allocator::AllocationType::Buffer,
    { .buffer = buffer },
    is_linear
  );

  {
    auto result = vkBindBufferMemory(
      device,
      buffer,
      alloc.memory,
      alloc.offset_start
    );
    assert(result == VK_SUCCESS);
  }

  VkDeviceAddress buffer_address = 0;
  if (info->usage == (info->usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
    VkBufferDeviceAddressInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buffer,
    };
    buffer_address = vkGetBufferDeviceAddress(device, &info);
    assert(buffer_address != 0);
  }

  return {
    .id = alloc.id,
    .buffer = buffer,
    .buffer_address = buffer_address,
  };
}

Image create_image(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VkPhysicalDeviceProperties *properties,
  VkImageCreateInfo *info
) {
  ZoneScoped;

  VkImage image;
  {
    auto result = vkCreateImage(
      device,
      info,
      allocator,
      &image
    );
    assert(result == VK_SUCCESS);
  }
  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, image, &requirements);

  bool is_linear = info->tiling == VK_IMAGE_TILING_LINEAR;
  // don't support this DRM format, too finicky
  assert(info->tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);

  auto alloc = _allocate(
    it,
    device,
    allocator,
    properties,
    &requirements,
    Allocator::AllocationType::Image,
    { .image = image },
    is_linear
  );

  {
    auto result = vkBindImageMemory(
      device,
      image,
      alloc.memory,
      alloc.offset_start
    );
    assert(result == VK_SUCCESS);
  }

  return {
    .id = alloc.id,
    .image = image,
  };
}

HostMapping get_host_mapping(
  Ref<Allocator> it,
  ID id
) {
  ZoneScoped;
  assert(id != 0);
  lib::mutex::lock(&it->mutex);

  if (id < 0) {
    // @Performance: could do binary search here.
    for (auto &item : it->dedicated_allocations) {
      if (item.id == id) {
        lib::mutex::unlock(&it->mutex);
        return {
          .mem = item.mem,
          .size = item.size,
        };
      }
    }
    assert("ID not found" && false);
  } else {
    Allocator::Region *region = nullptr;
    for (auto &item : it->regions) {
      if (item.first_id > id) {
        break;
      }
      region = &item;
    }
    assert(region != nullptr);
    // @Performance: could do binary search here.
    for (auto &item : region->suballocations) {
      if (item.id == id) {
        lib::mutex::unlock(&it->mutex);
        return {
          .mem = (uint8_t *) region->mem + item.offset,
          .size = item.size,
        };
      }
    }
    assert("ID not found" && false);
  }
}

void _deallocate(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  ID id
) {
  ZoneScoped;
  assert(id != 0);
  lib::mutex::lock(&it->mutex);

  if (id < 0) {
    bool did_erase = false;
    // @Performance: could do binary search here.
    for (size_t i = 0; i < it->dedicated_allocations.size(); i++) {
      if (it->dedicated_allocations[i].id == id) {
        vkFreeMemory(device, it->dedicated_allocations[i].memory, allocator);
        TracyFreeN((void *) it->dedicated_allocations[i].memory, it->debug_name);
        it->dedicated_allocations.erase(it->dedicated_allocations.begin() + i);
        did_erase = true;
        break;
      }
    }
    if (!did_erase) {
      assert("ID not found" && false);
    }
  } else {
    Allocator::Region *region = nullptr;
    size_t region_index = 0;
    for (size_t i = 0; i < it->regions.size(); i++) {
      if (it->regions[i].first_id > id) {
        break;
      }
      region = &it->regions[i];
      region_index = i;
    }
    assert(region != nullptr);
    // @Performance: could do binary search here.
    bool did_erase = false;
    for (size_t i = 0; i < region->suballocations.size(); i++) {
      if (region->suballocations[i].id == id) {
        region->suballocations.erase(region->suballocations.begin() + i);
        did_erase = true;
      }
    }

    if (!did_erase) {
      assert("ID not found" && false);
    }

    region->total_active_suballocations--;
    if (region->total_active_suballocations == 0) {
      vkFreeMemory(device, region->memory, allocator);
      TracyFreeN((void *) region->memory, it->debug_name);
      it->regions.erase(it->regions.begin() + region_index);
    }
  }

  lib::mutex::unlock(&it->mutex);
}

void destroy_buffer(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  Buffer buffer
) {
  ZoneScoped;

  vkDestroyBuffer(device, buffer.buffer, allocator);
  _deallocate(it, device, allocator, buffer.id);
}

void destroy_image(
  Ref<Allocator> it,
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  Image image
) {
  ZoneScoped;

  vkDestroyImage(device, image.image, allocator);
  _deallocate(it, device, allocator, image.id);
}

} // namespace