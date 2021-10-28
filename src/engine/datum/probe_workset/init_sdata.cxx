#pragma once
#include <vulkan/vulkan.h>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/session/data.hxx>
#include "constants.hxx"
#include "data.hxx"

namespace engine::datum::probe_workset {

void init_sdata(
  SData *out,
  Ref<engine::session::Vulkan::Core> core,
  Ref<lib::gfx::Allocator> allocator_device
) {
  ZoneScoped;

  std::vector<lib::gfx::allocator::Buffer> buffers_workset;
  for (size_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = MAX_COUNT * 16, // uvec4
      .usage = (0
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    auto buffer = lib::gfx::allocator::create_buffer(
      allocator_device.ptr,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );

    buffers_workset.push_back(buffer);
  }

  std::vector<lib::gfx::allocator::Buffer> buffers_counter;
  for (size_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizeof(VkDispatchIndirectCommand),
      .usage = (0
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    auto buffer = lib::gfx::allocator::create_buffer(
      allocator_device.ptr,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );

    buffers_counter.push_back(buffer);
  }

  *out = {
    .buffers_workset = std::move(buffers_workset),
    .buffers_counter = std::move(buffers_counter),
  };
}

} // namespace
