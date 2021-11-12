#pragma once
#include "base.hxx"

namespace lib::easy_allocator {

lib::allocator_t* create(size_t conceivable_size);
void destroy(lib::allocator_t *);

} // namespace