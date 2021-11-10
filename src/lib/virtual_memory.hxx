#pragma once

namespace lib::virtual_memory {

extern const char *tracy_pool_name;
size_t get_page_size();
void *reserve(size_t num_pages);
void commit(void *mem, size_t num_pages);
void free(void *mem);

} // namespace
