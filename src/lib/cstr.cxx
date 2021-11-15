#pragma once
#include <cstring>
#include "_base/allocator.hxx"
#include "cstr.hxx"

namespace lib::cstr {

cstr_range_t from_static(const char *str) {
  return cstr_range_t(str, str + strlen(str));
}

cstr_range_t crt_copy(cstr_range_t cstr) {
  auto length = cstr.end - cstr.start;
  auto start = (const char *) lib::allocator::crt->alloc_fn(
    lib::allocator::crt,
    length + 1, // trailing zero
    1
  );
  return cstr_range_t(start, start + length);
}

void crt_free(cstr_range_t cstr) {
  // can we detect static strings here?

  lib::allocator::crt->dealloc_fn(
    lib::allocator::crt,
    (void *) cstr.start
  );
}

const uint64_t fnv_basis = 14695981039346656037ull;
const uint64_t fnv_prime = 1099511628211ull;

u64_table::hash_t fnv_1a(cstr_range_t cstr) {
  uint64_t result = fnv_basis;
  auto ptr = cstr.start;
  while (ptr < cstr.end) {
    result = result ^ *ptr;
    result = result * fnv_prime;
    ptr++;
  }
  return u64_table::hash_t(result);
}

} // namespace
