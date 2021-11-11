#pragma once
#include "allocator.hxx"
#include "common.hxx"

// Inspired by stb's stretchy_buffer.

namespace lib {
  template<typename T>
  struct array_t {
    // @Think: does it make sense to move bookkeeping out, in order to
    // simplify the interface for `ensure_space` and the like?
    lib::allocator_t *acr;
    size_t count;
    size_t capacity;
    T data[];
  };
}

namespace lib::array {

template<typename T>
inline array_t<T> *create(
  lib::allocator_t *acr,
  size_t initial_capacity
) {
  auto size = sizeof(array_t<T>) + sizeof(T) * initial_capacity;
  auto it = (array_t<T> *) acr->alloc_fn(acr, size, alignof(array_t<T>));
  it->acr = acr;
  it->count = 0;
  it->capacity = initial_capacity;
  return it;
}

template<typename T>
inline void destroy(array_t<T> *it) {
  it->acr->free_fn(it->acr, it);
}

template<typename T>
inline array_t<T> *ensure_space(array_t<T> *it, size_t added_count) {
  if (it == nullptr || it->count + added_count > it->capacity) {
    size_t new_capacity = (
      it == nullptr
        ? added_count
        : max(it->capacity * 2, it->count + added_count)
    );
    auto new_size = sizeof(array_t<T>) + sizeof(T) * new_capacity;
    it = (array_t<T> *) it->acr->realloc_fn(it->acr, it, new_size, alignof(T));
    assert(it != nullptr);
    it->capacity = new_capacity;
  }
  return it;
}

} // namespace