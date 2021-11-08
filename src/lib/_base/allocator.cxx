#pragma once
#include <cstdlib>
#include "allocator.hxx"

namespace lib::allocator {

void *crt_alloc_fn(Allocator *it, size_t size) {
  return malloc(size);
}

void *crt_realloc_fn(Allocator *it, void *ptr, size_t size) {
  return realloc(ptr, size);
}

void crt_free_fn(Allocator *it, void *ptr) {
  free(ptr);
}

Allocator crt_data = {
  .alloc_fn = crt_alloc_fn,
  .realloc_fn = crt_realloc_fn,
  .free_fn = crt_free_fn,
};

Allocator *crt = &crt_data;

} // namespace
