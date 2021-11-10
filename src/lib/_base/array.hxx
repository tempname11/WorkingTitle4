#pragma once
#include "allocator.hxx"
#include "common.hxx"

// Inspired by stb stretchy_buffer.

namespace lib {
  template<typename T>
  struct array_t {
    lib::allocator_t *acr;
    size_t count;
    size_t reserved;
    T data[];
  };
}

namespace lib::array {

template<typename T>
inline array_t<T> *create(
  lib::allocator_t *acr,
  size_t initial_reserved
) {
  auto size = sizeof(array_t<T>) + sizeof(T) * initial_reserved;
  auto it = (array_t<T> *) acr->alloc_fn(acr, size, alignof(T));
  it->acr = acr;
  it->count = 0;
  it->reserved = initial_reserved;
  return it;
}

template<typename T>
inline void destroy(array_t<T> *it) {
  it->acr->free_fn(it->acr, it);
}

template<typename T>
inline array_t<T> *ensure_space(array_t<T> *it, size_t added_count) {
  if (it == nullptr || it->count + added_count > it->reserved) {
    size_t new_reserved = (
      it == nullptr
        ? added_count
        : max(it->reserved * 2, it->count + added_count)
    );
    auto new_size = sizeof(array_t<T>) + sizeof(T) * new_reserved;
    it = (array_t<T> *) it->acr->realloc_fn(it->acr, it, new_size, alignof(T));
    assert(it != nullptr);
    it->reserved = new_reserved;
  }
  return it;
}

} // namespace