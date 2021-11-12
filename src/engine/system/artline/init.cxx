#include <src/lib/mutex.hxx>
#include "public.hxx"
#include "data.hxx"

namespace engine::system::artline {

void init(Data *out) {
  *out = {};
  lib::mutex::init(&out->mutex);
  out->dlls = lib::u64_table::create<Data::DLL>(lib::allocator::crt, 0);
}

void deinit(Data *it) {
  lib::mutex::deinit(&it->mutex);
  lib::u64_table::destroy(it->dlls);
}

} // namespace
