#pragma once
#include "guid.hxx"
#include "cstr.hxx"

namespace lib {
  struct hash64_t {
    uint64_t as_number;

    // make C++ happy
    bool operator==(hash64_t h) const { return this->as_number == h.as_number; }
  };
}

namespace lib::hash64 {
  inline hash64_t from_guid(lib::GUID guid) {
    // No hashing on purpose. Handy for debug inspection,
    // but might cause problems in rare cases. @Think
    return hash64_t(guid);
  }

  extern inline uint64_t begin();
  extern inline void add_bytes(uint64_t *h, void *ptr, size_t size);
  extern inline hash64_t get(uint64_t *h);

  inline hash64_t from_bytes(void *ptr, size_t size) {
    uint64_t h = begin();
    add_bytes(&h, ptr, size);
    return hash64_t(h);
  }

  template<typename T>
  inline hash64_t from_memory_range(memory_range_t<T> range) {
    return from_bytes((void *) range.start, (range.end - range.start) * sizeof(T));
  }

  inline hash64_t from_cstr(cstr_range_t cstr) {
    return from_memory_range(cstr);
  }

  template<typename T>
  inline hash64_t from_value(T t) {
    return from_bytes(&t, sizeof(T));
  }

  template<typename T>
  inline void add_memory_range(uint64_t *h, memory_range_t<T> range) {
    add_bytes(h, range.start, (range.end - range.start) * sizeof(T));
  }

  template<typename T>
  inline void add_cstr(uint64_t *h, cstr_range_t range) {
    add_memory_range(h, cstr);
  }

  template<typename T>
  inline void add_value(uint64_t *h, T t) {
    add_bytes(h, &t, sizeof(T));
  }
}