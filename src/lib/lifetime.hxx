#pragma once
#include <atomic>
#include <cstdint>
#include "task.hxx"

namespace lib {
  struct Lifetime {
    std::atomic<size_t> ref_count;
    lib::Task *yarn;

    // C++ doesn't allow copying atomics by default,
    // so we need explicit zero constructors
    Lifetime();
    Lifetime(const Lifetime &other);
    Lifetime &operator= (const Lifetime &other);
  };
}

namespace lib::lifetime {
  void ref(Lifetime *lifetime);
  bool deref(Lifetime *lifetime, lib::task::Runner *runner);
  void init(Lifetime *lifetime, size_t initial_count);
}