#include <cassert>
#include "hash64.hxx"

namespace lib::hash64 {

// FNV-1a, a super simple thing that seems OK for our purposes.

const uint64_t fnv_basis = 14695981039346656037ull;
const uint64_t fnv_prime = 1099511628211ull;

inline uint64_t begin() {
  return fnv_basis;
}

inline void add_bytes(uint64_t *h, void *ptr, size_t size) {
  // also use size
  *h = *h ^ size;
  *h = *h * fnv_prime;

  auto p = (uint8_t *) ptr;
  auto end = p + size;
  while (p < end) {
    *h = *h ^ *p;
    *h = *h * fnv_prime;
    p++;
  }
}

inline hash64_t get(uint64_t *h) {
  assert(*h != 0);
  assert(*h != fnv_basis);
  return hash64_t(*h);
}

} // namespace