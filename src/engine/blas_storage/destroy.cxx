#include "../blas_storage.hxx"
#include "data.hxx"

namespace engine::blas_storage {

void destroy(
  Ref<BlasStorage> it,
  Ref<engine::session::Vulkan::Core> core,
  ID id
) {
  ZoneScoped;

  std::unique_lock lock(it->rw_mutex);
  auto item = &it->items.at(id);

  // @Incomplete: could wait for Ready state here?
  assert(item->status == BlasStorage::ItemData::Status::Ready);

  auto ext = &core->extension_pointers;
  ext->vkDestroyAccelerationStructureKHR(
    core->device,
    item->accel,
    core->allocator
  );

  lib::gfx::allocator::destroy_buffer(
    &it->allocator_blas,
    core->device,
    core->allocator,
    item->buffer
  );

  it->items.erase(id);
}

} // namespace
