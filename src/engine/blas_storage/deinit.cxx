#include "../blas_storage.hxx"
#include "data.hxx"

namespace engine::blas_storage {

void deinit(
  BlasStorage *it,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  lib::gfx::allocator::deinit(
    &it->allocator_blas,
    core->device,
    core->allocator
  );

  lib::gfx::allocator::deinit(
    &it->allocator_scratch,
    core->device,
    core->allocator
  );

  vkDestroyCommandPool(
    core->device,
    it->command_pool,
    core->allocator
  );
}

} // namespace
