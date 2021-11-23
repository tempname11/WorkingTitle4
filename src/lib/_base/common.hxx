#pragma once

#ifdef min
  #undef min
#endif

#ifdef max
  #undef max
#endif

namespace lib {
  template<typename T>
  T min(T a, T b) {
    return a < b ? a : b;
  }

  template<typename T>
  T max(T a, T b) {
    return a > b ? a : b;
  }

  const double PI = 3.14159265358979323846;
}
