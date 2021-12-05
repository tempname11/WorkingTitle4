#pragma once
#include "base.hxx"

namespace lib::easy_allocator {

// Good for many short-lived, small allocations,
// which then are all released at once. Thread-safe.
lib::allocator_t* create(size_t conceivable_size);
void destroy(lib::allocator_t *);

} // namespace