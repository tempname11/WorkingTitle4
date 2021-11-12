#pragma once
#include <cstdint>

namespace lib {
  template<typename T>
  struct memory_range_t {
    T *start;
    T *end;

    inline memory_range_t(T* start, T* end) { // prevent default constructor
      this->start = start;
      this->end = end;
    }
  };

  using byte_range_t = memory_range_t<uint8_t>;
}
