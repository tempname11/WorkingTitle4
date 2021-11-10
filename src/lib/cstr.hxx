#pragma once
#include "_base/memory_range.hxx"

namespace lib {
  using cstr_range_t = memory_range_t<char const>;
}

namespace lib::cstr {
  cstr_range_t from_static(const char *str);
}
