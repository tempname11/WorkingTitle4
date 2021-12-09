#include <cstddef>
#include <cstring>
#include <Tracy.hpp>
#include "base.hxx"
#include "mutex.hxx"
#include "virtual_memory.hxx"

namespace lib::easy_allocator {

struct implementation_t {
  allocator_t parent;
  mutex_t mutex;
  size_t reserved_pages;
  size_t committed_pages;
  size_t free_range_start_offset;
};

// Reserved before every allocation.
struct reserved_t {
  size_t size;
};

implementation_t *cast(allocator_t *parent) {
  return (implementation_t *) (
    ((uint8_t *) parent) - offsetof(implementation_t, parent)
  );
}

void *alloc_fn(
  allocator_t *parent,
  size_t size,
  size_t alignment
) {
  auto it = cast(parent);
  auto page_size = virtual_memory::get_page_size();
  lib::mutex::lock(&it->mutex);
  
  // Reserve memory for a `reserved_t`.
  // The final location will always be just behind `ptr`.
  auto reserved_intermediate_offset = alignof(reserved_t) * (
    (it->free_range_start_offset + alignof(reserved_t) - 1) / alignof(reserved_t)
  );
  auto aligned_start_offset = alignment * (
    ((reserved_intermediate_offset + sizeof(reserved_t)) + alignment - 1) / alignment
  );
  auto end_offset = aligned_start_offset + size;
  auto free_range_end_offset = it->committed_pages * page_size;

  if (end_offset > free_range_end_offset) {
    auto added_size = end_offset - free_range_end_offset;
    auto added_pages = (added_size + page_size - 1) / page_size;
    auto old_committed_pages = it->committed_pages;

    it->committed_pages += added_pages;
    assert(it->committed_pages <= it->reserved_pages);
    virtual_memory::commit((void *) it, it->committed_pages);

    // We do this in a loop, because otherwise we'd need to track page indices 
    // and counts for `TracyFreeN`. :TrackPagesForTracy
    for (auto i = 0; i < added_pages; i++) {
      TracyAllocN(
        (uint8_t *)(it) + page_size * (old_committed_pages + i),
        page_size,
        virtual_memory::tracy_pool_name
      );
    }
  }

  it->free_range_start_offset = end_offset;
  lib::mutex::unlock(&it->mutex);

  auto ptr = (uint8_t *) it + aligned_start_offset;

  // Don't use `reserved_intermediate_offset` here. `reserved` will still be
  // properly aligned, regardless of the `alignment`.
  auto reserved = (reserved_t *) (ptr - sizeof(size_t));
  *reserved = {
    .size = size,
  };

  return (void *) ptr;
}

void *realloc_fn(
  allocator_t *parent,
  void *ptr,
  size_t size,
  size_t alignment
) {
  auto reserved = (reserved_t *) ((uint8_t *) ptr - sizeof(size_t));
  void *new_ptr = alloc_fn(parent, size, alignment);
  memcpy(new_ptr, ptr, reserved->size);
  return new_ptr;
}

void dealloc_fn(allocator_t *parent, void *ptr) {
  assert("Not supported" && false);
}

allocator_t* create(size_t conceivable_size) {
  auto page_size = virtual_memory::get_page_size();
  assert(conceivable_size > sizeof(implementation_t));
  assert(page_size > sizeof(implementation_t));

  auto reserved_pages = (conceivable_size + page_size - 1) / page_size;
  size_t committed_pages = 1;
  auto mem = virtual_memory::reserve(reserved_pages);
  virtual_memory::commit(mem, committed_pages);

  TracyAllocN(
    mem,
    page_size,
    virtual_memory::tracy_pool_name
  );

  auto it = (implementation_t *) mem;
  *it = implementation_t {
    .parent = {
      .alloc_fn = alloc_fn,
      .realloc_fn = realloc_fn,
      .dealloc_fn = dealloc_fn,
    },
    .reserved_pages = reserved_pages,
    .committed_pages = committed_pages,
    .free_range_start_offset = sizeof(implementation_t),
  };
  lib::mutex::init(&it->mutex);

  return &it->parent;
}

void destroy(lib::allocator_t *parent) {
  auto it = cast(parent);
  auto page_size = virtual_memory::get_page_size();
  lib::mutex::deinit(&it->mutex);
  for (size_t i = 0; i < it->committed_pages; i++) {
    TracyFreeN(
      (uint8_t *)(it) + page_size * i,
      virtual_memory::tracy_pool_name
    );
  }
  virtual_memory::free((void *) it);
}

} // namespace