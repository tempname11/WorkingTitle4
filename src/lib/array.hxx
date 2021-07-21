#pragma once

namespace lib::array {

  template<typename T>
  struct Array;

  template<typename T>
  Array<T> *create(size_t max_reserved);

} // namespace

namespace lib {
  template<typename T>
  using Array = lib::array::Array<T>;
}

#include "array.inline.hxx"