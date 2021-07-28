#pragma once
#include <atomic>
#include <cstdint>

namespace lib {
  using GUID = uint64_t;
}

namespace lib::guid {
  struct Counter {
    std::atomic_uint64_t next;
  };

  GUID next(Counter *it);
} // namespace