#pragma once
#include <cstdint>

namespace lib {

struct Allocator;

namespace allocator {
  using AllocFn = void * (Allocator *it, size_t size);
  using ReallocFn = void * (Allocator *it, void *ptr, size_t size);
  using FreeFn = void (Allocator *it, void *ptr);
}

struct Allocator {
  allocator::AllocFn *alloc_fn;
  allocator::ReallocFn *realloc_fn;
  allocator::FreeFn *free_fn;
  uint8_t custom_data[];
};

namespace allocator {
  extern Allocator *crt;
}

} // namespace
