#pragma once
#include <atomic>
#include <cstdint>

namespace lib {
  using GUID = uint64_t;
  // @Cleanup: use `struct guid_t { uint64_t as_number; };` in the future.
}

namespace lib::guid {
  struct Counter {
    std::atomic<uint64_t> next;
  };

  GUID next(Counter *it);
}