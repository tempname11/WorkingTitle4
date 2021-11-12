#pragma once
#include "allocator.hxx"
#include "common.hxx"

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
inline void ensure_space(array_t<T> **p_it, size_t added_count) {
  auto it = *p_it;
  assert(it != nullptr);
  if (it->count + added_count > it->capacity) {
    size_t new_capacity = max(it->capacity * 2, it->count + added_count);
    auto new_size = sizeof(array_t<T>) + sizeof(T) * new_capacity;
    it = (array_t<T> *) it->acr->realloc_fn(it->acr, it, new_size, alignof(T));
    assert(it != nullptr);
    it->capacity = new_capacity;
    *p_it = it;
  }
}

} // namespace