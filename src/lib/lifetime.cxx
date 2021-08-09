#include <cassert>
#include <src/global.hxx>
#include "lifetime.hxx"

namespace lib {
  Lifetime::Lifetime() {
    ref_count = 0;
    yarn = nullptr;
  }

  Lifetime::Lifetime(const Lifetime &other) {
    assert(other.ref_count == 0);
    ref_count = 0;
    yarn = nullptr;
  }
}

namespace lib::lifetime {

void ref(Lifetime *lifetime) {
  /*auto fetched =*/ lifetime->ref_count.fetch_add(1);
}

bool deref(Lifetime *lifetime, lib::task::Runner *runner) {
  auto fetched = lifetime->ref_count.fetch_sub(1);
  assert(fetched != 0);
  auto final = fetched == 1;
  if (final) {
    // since the caller is the only owner, it's safe to write to memory here.
    auto yarn = lifetime->yarn;
    lifetime->yarn = nullptr;

    lib::task::signal(runner, yarn);
  }
  return final;
}

void init(Lifetime *lifetime) {
  assert(lifetime->yarn == nullptr);
  lifetime->ref_count = 1;
  lifetime->yarn = lib::task::create_yarn_signal();
}

} // namespace