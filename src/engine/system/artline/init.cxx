#include <src/lib/mutex.hxx>
#include "public.hxx"
#include "data.hxx"

namespace engine::system::artline {

void init(Data *out) {
  *out = {};
  lib::mutex::init(&out->mutex);
}

void deinit(Data *it) {
  lib::mutex::deinit(&it->mutex);
}

} // namespace
