#pragma once
#include <cstdint>

namespace lib {
  template<typename T>
  struct memory_range_t {
    T *start;
    T *end;
  };

  using byte_range_t = memory_range_t<uint8_t>;
}
