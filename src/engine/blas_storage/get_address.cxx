#include "../blas_storage.hxx"
#include "data.hxx"

namespace engine::blas_storage {

VkDeviceAddress get_address(
  Ref<BlasStorage> it,
  ID id
) {
  ZoneScoped;

  std::unique_lock lock(it->rw_mutex);
  return it->items.at(id).accel_address;
}

} // namespace
