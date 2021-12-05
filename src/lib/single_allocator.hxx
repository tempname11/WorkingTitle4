#pragma once
#include "base.hxx"

namespace lib::single_allocator {

// Only supports a single allocation, but guarantees that realloc
// will not move memory and will be constant time. Good for "infinite" arrays.
// Not thread-safe, because you typically synchronise the "single object" itself.
// Destroyed, when the single allocation is deallocated.

lib::allocator_t* create(size_t conceivable_size);

} // namespace