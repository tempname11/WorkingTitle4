#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine {
  struct BlasStorage {
    struct ItemData {
      enum Status {
        Building,
        Ready,
      };

      Status status;
      lib::gfx::allocator::Buffer buffer;
      VkAccelerationStructureKHR accel;
      VkDeviceAddress accel_address;
    };

    std::shared_mutex rw_mutex;
    uploader::ID last_id;
    std::unordered_map<uploader::ID, ItemData> items;
    VkCommandPool command_pool;

    lib::gfx::Allocator allocator_blas;
    lib::gfx::Allocator allocator_scratch;
  };
}
