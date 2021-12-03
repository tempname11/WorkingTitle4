#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "blas_storage/id.hxx"

namespace engine {
  struct BlasStorage;
}

namespace engine::blas_storage {

void init(
  BlasStorage *it,
  Ref<engine::session::VulkanData::Core> core,
  size_t size_region
);

void deinit(
  BlasStorage *it,
  Ref<engine::session::VulkanData::Core> core
);

struct VertexInfo {
  VkDeviceSize stride;
  uint32_t index_count;
  uint32_t buffer_offset_indices;
  uint32_t buffer_offset_vertices;
  VkBuffer buffer;
};

ID create(
  lib::task::ContextBase *ctx,
  Ref<BlasStorage> it,
  lib::Task *signal,
  Use<VertexInfo> vertex_info,
  Ref<engine::session::Data> session,
  Ref<engine::session::VulkanData::Core> core
);

void destroy(
  Ref<BlasStorage> it,
  Ref<engine::session::VulkanData::Core> core,
  ID id
);

VkDeviceAddress get_address(
  Ref<BlasStorage> it,
  ID id
);

} // namespace
