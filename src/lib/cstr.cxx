#pragma once
#include <cstring>
#include "_base/allocator.hxx"
#include "cstr.hxx"

namespace lib::cstr {

cstr_range_t from_static(const char *str) {
  return cstr_range_t(str, str + strlen(str));
}

cstr_range_t crt_copy(cstr_range_t cstr) {
  auto size = size_t(cstr.end - cstr.start);
  auto ptr = (char *) lib::allocator::crt->alloc_fn(
    lib::allocator::crt,
    size + 1, // trailing zero
    1
  );
  memcpy((void *) ptr, cstr.start, size);
  ptr[size] = 0;
  return cstr_range_t(ptr, ptr + size);
}

void crt_free(cstr_range_t cstr) {
  // can we detect static strings here?

  lib::allocator::crt->dealloc_fn(
    lib::allocator::crt,
    (void *) cstr.start
  );
}

} // namespace
