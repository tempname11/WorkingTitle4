#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>

namespace engine::uploader {
  using ID = uint64_t;
}

namespace engine {
  struct Uploader {
    struct BufferData {
      enum class Status {
        New,
        Uploading,
        Ready
      };

      Status status;
      lib::gfx::allocator::Buffer buffer_host;
      lib::gfx::allocator::Buffer buffer_device;
      size_t size;
    };

    std::shared_mutex rw_mutex;
    uploader::ID last_id;
    std::unordered_map<uploader::ID, BufferData> buffers;
    VkCommandPool command_pool;

    lib::gfx::Allocator allocator_host;
    lib::gfx::Allocator allocator_device;
  };
}
