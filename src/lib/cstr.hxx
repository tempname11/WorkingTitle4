#pragma once
#include "_base/memory_range.hxx"
#include "u64_table.hxx"

namespace lib {
  using cstr_range_t = memory_range_t<char const>; // has a C-convention trailing zero at [end].
}

namespace lib::cstr {
  cstr_range_t from_static(const char *str);
  cstr_range_t crt_copy(cstr_range_t cstr);
  void crt_free(cstr_range_t cstr);
  u64_table::hash_t fnv_1a(cstr_range_t cstr);
}
