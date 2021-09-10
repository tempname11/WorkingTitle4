#include "../blas_storage.hxx"
#include "data.hxx"

namespace engine::blas_storage {

void init(
  BlasStorage *it,
  Use<SessionData::Vulkan::Core> core,
  size_t size_region
) {
  ZoneScoped;

  lib::gfx::allocator::init(
    &it->allocator_scratch,
    &core->properties.memory,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    size_region,
    "blas_storage.allocator_scratch"
  );

  lib::gfx::allocator::init(
    &it->allocator_blas,
    &core->properties.memory,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    size_region,
    "blas_storage.allocator_blas"
  );

  { ZoneScopedN("command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = core->queue_family_index,
    };
    auto result = vkCreateCommandPool(
      core->device,
      &create_info,
      core->allocator,
      &it->command_pool
    );
    assert(result == VK_SUCCESS);
  }
}

} // namespace
