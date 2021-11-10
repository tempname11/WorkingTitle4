#pragma once
#include <cstdint>

namespace lib {

struct allocator_t;

namespace allocator {
  using alloc_fn_t = void * (
    allocator_t *it,
    size_t size,
    size_t alignment
  );
  using realloc_fn_t = void * (
    allocator_t *it,
    void *ptr,
    size_t size,
    size_t alignment
  );
  using dealloc_fn_t = void (
    allocator_t *it,
    void *ptr
  );
}

struct allocator_t {
  allocator::alloc_fn_t *alloc_fn;
  allocator::realloc_fn_t *realloc_fn;
  allocator::dealloc_fn_t *dealloc_fn;
};

namespace allocator {
  extern allocator_t *crt;
}

} // namespace
