#pragma once
#include "allocator.hxx"

// Inspired by stb stretchy_buffer.

namespace lib {
  template<typename T>
  struct Array {
    lib::Allocator *acr;
    size_t count;
    size_t reserved;
    T data[];
  };
}

namespace lib::array {

template<typename T>
inline Array<T> *create(
  lib::Allocator *acr,
  size_t initial_reserved
) {
  auto size = sizeof(Array<T>) + sizeof(T) * initial_reserved;
  auto it = (Array<T> *) acr->alloc_fn(acr, size);
  it->acr = acr;
  it->count = 0;
  it->reserved = initial_reserved;
  return it;
}

template<typename T>
inline void destroy(Array<T> *it) {
  it->acr->free_fn(it->acr, it);
}

template<typename T>
inline void add(Array<T> *it, size_t added_count) {
  if (it == nullptr || it->count + added_count > it->reserved) {
    size_t new_reserved = (
      it == nullptr
        ? added_count
        : max(it->reserved * 2, it->count + added_count)
    );
    auto new_size = sizeof(Array<T>) + sizeof(T) * new_reserved;
    it = (Array<T> *) it->acr.realloc_fn(it->acr, it, new_size);
    assert(it != nullptr);
    it->reserved = new_reserved;
  }
  it->count += added_count;
}

} // namespace