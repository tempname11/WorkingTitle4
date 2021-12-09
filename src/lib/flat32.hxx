#pragma once
#include "base.hxx"
#include "single_allocator.hxx"

namespace lib {
  template<typename T>
  struct flat32_t {
    lib::array_t<T> *values;
    lib::array_t<uint32_t> *free_indices;
    
  };
}

// A common pattern: a contiguous array of values, and an array of free indexes,
// which is needed to remove values from the middle without moving other data.

namespace lib::flat32 {

const uint32_t MAX_INDEX = ~uint32_t(0);
template<typename T>
flat32_t<T> create() {
  return flat32_t<T> {
    .values = lib::array::create<T>(lib::single_allocator::create(
      lib::allocator::GB * sizeof(T)
    ), 0),
    .free_indices = lib::array::create<uint32_t>(lib::single_allocator::create(
      lib::allocator::GB * sizeof(uint32_t)
    ), 0),
  };
}

template<typename T>
void destroy(flat32_t<T> *it) {
  lib::array::destroy(it->values);
  lib::array::destroy(it->free_indices);
}

template<typename T>
uint32_t add(flat32_t<T> *it) {
  if (it->free_indices->count > 0) {
    return it->free_indices->data[--it->free_indices->count];
  }   

  lib::array::ensure_space(&it->values, 1);
  assert(it->values->count <= MAX_INDEX);
  return uint32_t(it->values->count++);
}

template<typename T>
void remove(flat32_t<T> *it, uint32_t ix) {
  if (ix == it->values->count - 1) {
    it->values->count--;
  } else {
    lib::array::ensure_space(&it->free_indices, 1);
    it->free_indices->data[it->free_indices->count++] = ix;
  }
}

} // namespace