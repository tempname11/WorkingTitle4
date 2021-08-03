#include <cassert>
#include <src/global.hxx>
#include "lifetime.hxx"

namespace lib::lifetime {

void ref(Lifetime *lifetime) {
  /*auto fetched =*/ lifetime->ref_count.fetch_add(1);
}

void deref(Lifetime *lifetime, lib::task::Runner *runner) {
  auto fetched = lifetime->ref_count.fetch_sub(1);
  if (fetched == 1) {
    lib::task::signal(runner, lifetime->yarn);
  }
}

void init(Lifetime *lifetime) {
  assert(lifetime->yarn == nullptr);
  lifetime->ref_count = 1;
  lifetime->yarn = lib::task::create_yarn_signal();
}

} // namespace