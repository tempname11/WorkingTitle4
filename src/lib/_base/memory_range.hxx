#pragma once
#include <cstdint>

namespace lib {
  template<typename T>
  struct memory_range_t {
    T *start;
    T *end;

    // C++ stuff:
    // prevent default constructor, so that `memory_range_t(T*)`
    // does not construct an obviously invalid range.
    inline memory_range_t() {
      this->start = nullptr;
      this->end = nullptr;
    }
    inline memory_range_t(T* start, T* end) {
      this->start = start;
      this->end = end;
    }
  };

  using byte_range_t = memory_range_t<uint8_t>;
}
