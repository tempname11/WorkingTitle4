#include <src/lib/defer.hxx>
#include "../blas_storage.hxx"
#include "data.hxx"

namespace engine::blas_storage {

struct BuildSubmitData {
  ID id;
  VkCommandBuffer cmd;
  VkSemaphore semaphore;
  lib::gfx::allocator::Buffer buffer_scratch;
};

void _build_submit(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<VkQueue> queue_work,
  Own<BuildSubmitData> data
) {
  ZoneScoped;

  uint64_t one = 1;
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &one,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &data->semaphore,
  };
  auto result = vkQueueSubmit(
    *queue_work,
    1,
    &submit_info,
    VK_NULL_HANDLE
  );
  assert(result == VK_SUCCESS);
}

void _build_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<BlasStorage> it,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Own<BuildSubmitData> data
) {
  ZoneScoped;

  vkDestroySemaphore(
    core->device,
    data->semaphore,
    core->allocator
  );

  {
    std::unique_lock lock(it->rw_mutex);

    auto item = &it->items.at(data->id);
    item->status = BlasStorage::ItemData::Status::Ready;

    vkFreeCommandBuffers(
      core->device,
      it->command_pool,
      1,
      &data->cmd
    );

    lib::gfx::allocator::destroy_buffer(
      &it->allocator_scratch,
      core->device,
      core->allocator,
      data->buffer_scratch
    );
  }

  delete data.ptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

 BlasStorage::ItemData _build(
  lib::task::ContextBase *ctx,
  BlasStorage *it,
  lib::Task *signal,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core,
  Use<VertexInfo> vertex_info,
  ID id
) {
  auto ext = &core->extension_pointers;

  VkDeviceAddress buffer_address = 0;
  {
    VkBufferDeviceAddressInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = vertex_info->buffer,
    };
    buffer_address = vkGetBufferDeviceAddress(core->device, &info);
    assert(buffer_address != 0);
  }

  VkAccelerationStructureGeometryKHR geometry = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
    .geometry = {
      .triangles = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData = {
          .deviceAddress = buffer_address + vertex_info->buffer_offset_vertices,
        },
        .vertexStride = vertex_info->stride,
        .maxVertex = vertex_info->index_count,
        .indexType = VK_INDEX_TYPE_UINT16,
        .indexData = {
          .deviceAddress = buffer_address + vertex_info->buffer_offset_indices,
        },
      },
    },
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
  };

	VkAccelerationStructureBuildSizesInfoKHR sizes = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };

  {
    VkAccelerationStructureBuildGeometryInfoKHR geometry_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .geometryCount = 1,
      .pGeometries = &geometry,
    };
    ext->vkGetAccelerationStructureBuildSizesKHR(
      core->device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &geometry_info,
      &vertex_info->index_count,
      &sizes
    );
  }

  lib::gfx::allocator::Buffer buffer;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizes.accelerationStructureSize,
      .usage = (0
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    buffer = lib::gfx::allocator::create_buffer(
      &it->allocator_blas,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  lib::gfx::allocator::Buffer buffer_scratch;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizes.buildScratchSize,
      .usage = (0
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
      ),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer_scratch = lib::gfx::allocator::create_buffer(
      &it->allocator_scratch,
      core->device,
      core->allocator,
      &core->properties.basic,
      &info
    );
  }

  VkAccelerationStructureKHR accel;
  {
    VkAccelerationStructureCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = buffer.buffer,
      .offset = 0,
      .size = sizes.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };
    auto result = ext->vkCreateAccelerationStructureKHR(
      core->device,
      &create_info,
      core->allocator,
      &accel
    );
    assert(result == VK_SUCCESS);
  }

  VkSemaphore semaphore;
  { ZoneScopedN("semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      core->device,
      &create_info,
      core->allocator,
      &semaphore
    );
    assert(result == VK_SUCCESS);
  }

  VkCommandBuffer cmd;
  { ZoneScopedN("allocate_command_buffer");
    VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = it->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    {
      auto result = vkAllocateCommandBuffers(
        core->device,
        &alloc_info,
        &cmd
      );
      assert(result == VK_SUCCESS);
    }
    ZoneValue(uint64_t(cmd));
  }

  {
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }

  VkDeviceAddress scratch_address = 0;
  {
    VkBufferDeviceAddressInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buffer_scratch.buffer,
    };
    scratch_address = vkGetBufferDeviceAddress(core->device, &info);
    assert(scratch_address != 0);
  }

  {
    VkAccelerationStructureBuildGeometryInfoKHR geometry_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .dstAccelerationStructure = accel,
      .geometryCount = 1,
      .pGeometries = &geometry,
      .scratchData = scratch_address,
    };
    VkAccelerationStructureBuildRangeInfoKHR range_info = {
      .primitiveCount = vertex_info->index_count / 3, // triangles!
    };
    VkAccelerationStructureBuildRangeInfoKHR *range_infos[] = { &range_info };
    ext->vkCmdBuildAccelerationStructuresKHR(
      cmd,
      1,
      &geometry_info,
      range_infos
    );
  }

  {
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }

  lib::gpu_signal::associate(
    &session->gpu_signal_support,
    signal,
    core->device,
    semaphore,
    1
  );

  auto data = new BuildSubmitData {
    .id = id,
    .cmd = cmd,
    .semaphore = semaphore,
    .buffer_scratch = buffer_scratch,
  };

  auto task_build_submit = lib::task::create(
    _build_submit,
    &session->vulkan.queue_work,
    data
  );

  // @Note: passing `it` to the task is bit fishy,
  // because it relies on it having at least the lifetime of `session`.
  // Which is how it is currently used, but there's no rule or convention
  // preventing it to be used separately.
  lib::lifetime::ref(&session->lifetime);
  auto task_build_finish = lib::defer(
    lib::task::create(
      _build_finish,
      it,
      session.ptr,
      &session->vulkan.core,
      data
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_build_submit,
    task_build_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { signal, task_build_finish.first },
  });

  VkDeviceAddress accel_address = 0;
  {
    VkAccelerationStructureDeviceAddressInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
      .accelerationStructure = accel,
    };
    accel_address = ext->vkGetAccelerationStructureDeviceAddressKHR(core->device, &info);
    assert(accel_address != 0);
  }


  return BlasStorage::ItemData {
    .status = BlasStorage::ItemData::Status::Building,
    .buffer = buffer,
    .accel = accel,
    .accel_address = accel_address,
  };
}

ID create(
  lib::task::ContextBase *ctx,
  Ref<BlasStorage> it,
  lib::Task *signal,
  Use<VertexInfo> vertex_info,
  Ref<engine::session::Data> session,
  Use<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  ID id = 0;
  {
    std::unique_lock lock(it->rw_mutex);

    it->last_id++;
    id = it->last_id;

    auto item_data = _build(
      ctx,
      it.ptr,
      signal,
      session,
      core,
      vertex_info,
      id
    );

    it->items.insert({
      id,
      item_data,
    });
  }
  assert(id != 0);

  return id;
}

} // namespace
