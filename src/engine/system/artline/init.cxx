#include <src/lib/mutex.hxx>
#include "public.hxx"
#include "data.hxx"

namespace engine::system::artline {

void init(Data *out) {
  *out = {};
  lib::mutex::init(&out->mutex);
  out->dlls = lib::array::create<Data::DLL>(lib::allocator::crt, 0);
  out->dll_indices = lib::u64_table::create<size_t>(lib::allocator::crt, 0);
  out->meshes_by_key = lib::u64_table::create<CachedMesh>(lib::allocator::crt, 0);
  out->textures_by_key = lib::u64_table::create<CachedTexture>(lib::allocator::crt, 0);
}

void deinit(Data *it) {
  lib::mutex::deinit(&it->mutex);
  lib::array::destroy(it->dlls);
  lib::u64_table::destroy(it->dll_indices);
  lib::u64_table::destroy(it->meshes_by_key);
  lib::u64_table::destroy(it->textures_by_key);
}

} // namespace
