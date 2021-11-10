#pragma once
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include "allocator.hxx"

namespace lib::allocator {

void *crt_alloc_fn(
  allocator_t *it,
  size_t size,
  size_t alignment
) {
  assert(alignment <= alignof(max_align_t));
  return malloc(size);
}

void *crt_realloc_fn(
  allocator_t *it,
  void *ptr,
  size_t size,
  size_t alignment
) {
  assert(alignment <= alignof(max_align_t));
  return realloc(ptr, size);
}

void crt_dealloc_fn(allocator_t *it, void *ptr) {
  free(ptr);
}

allocator_t crt_data = {
  .alloc_fn = crt_alloc_fn,
  .realloc_fn = crt_realloc_fn,
  .dealloc_fn = crt_dealloc_fn,
};

allocator_t *crt = &crt_data;

} // namespace
