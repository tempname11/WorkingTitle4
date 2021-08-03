#pragma once
#include <atomic>
#include <cstdint>
#include "task.hxx"

namespace lib {
  struct Lifetime {
    std::atomic<size_t> ref_count;
    lib::Task *yarn;
  };
}

namespace lib::lifetime {
  void ref(Lifetime *lifetime);
  void deref(Lifetime *lifetime, lib::task::Runner *runner);
  void init(Lifetime *lifetime);
}