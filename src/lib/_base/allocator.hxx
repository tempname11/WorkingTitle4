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

  template<typename T>
  inline T *make(allocator_t *acr) {
    return (T *) acr->alloc_fn(acr, sizeof(T), alignof(T));
  }

  template<typename T>
  inline void forget(allocator_t *acr, T *ptr) {
    acr->dealloc_fn(acr, (void *) ptr);
  }

  const size_t KB = 1024;
  const size_t MB = 1024 * KB;
  const size_t GB = 1024 * MB;
}

} // namespace
