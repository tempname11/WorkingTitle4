#pragma once
#include <cstring>
#include "cstr.hxx"

namespace lib::cstr {

cstr_range_t from_static(const char *str) {
  return cstr_range_t {
    .start = str,
    .end = str + strlen(str),
  };
}

} // namespace
