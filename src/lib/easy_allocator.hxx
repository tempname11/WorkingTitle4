#pragma once
#include "base.hxx"

namespace lib::easy_allocator {

lib::allocator_t* create();
void destroy(lib::allocator_t *);

} // namespace