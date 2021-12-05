#include <cstddef>
#include <Tracy.hpp>
#include "base.hxx"
#include "virtual_memory.hxx"

namespace lib::single_allocator {

struct implementation_t {
  allocator_t parent;
  size_t reserved_pages;
  size_t committed_pages;
  void *allocated;
};

implementation_t *cast(allocator_t *parent) {
  return (implementation_t *) (
    ((uint8_t *) parent) - offsetof(implementation_t, parent)
  );
}

void *adjust(
  allocator_t *parent,
  size_t size,
  size_t alignment
) {
  auto it = cast(parent);
  auto page_size = virtual_memory::get_page_size();

  auto aligned_start_offset = alignment * (
    (sizeof(implementation_t) + alignment - 1) / alignment
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
        it->mem + page_size * (old_committed_pages + i),
        page_size,
        virtual_memory->tracy_pool_name
      );
    }
  }

  // @Incomplete: we could also free pages if new `size` is small.

  auto ptr = (uint8_t *) it + aligned_start_offset;
  it->allocated = (void *) ptr;
  return (void *) ptr;
}

void *alloc_fn(
  allocator_t *parent,
  size_t size,
  size_t alignment
) {
  auto it = cast(parent);
  assert(it->allocated == nullptr);
  return adjust(parent, size, alignment);
}

void *realloc_fn(
  allocator_t *parent,
  void *ptr,
  size_t size,
  size_t alignment
) {
  auto it = cast(parent);
  assert(it->allocated == ptr);
  return adjust(parent, size, alignment);
}

void dealloc_fn(allocator_t *parent, void *ptr) {
  auto it = cast(parent);
  assert(it->allocated == ptr);
  for (size_t i = 0; i < it->committed_pages; i++) {
    TracyFreeN(
      it->mem + page_size * i,
      virtual_memory->tracy_pool_name
    );
  }
  virtual_memory::free((void *) it);
}

allocator_t* create(size_t conceivable_size) {
  auto page_size = virtual_memory::get_page_size();
  assert(conceivable_size > sizeof(implementation_t));
  assert(page_size > sizeof(implementation_t));

  auto reserved_pages = (conceivable_size + page_size - 1) / page_size;
  size_t committed_pages = 1;
  auto mem = virtual_memory::reserve(reserved_pages);
  virtual_memory::commit(mem, committed_pages);

  auto it = (implementation_t *) mem;
  *it = implementation_t {
    .parent = {
      .alloc_fn = alloc_fn,
      .realloc_fn = realloc_fn,
      .dealloc_fn = dealloc_fn,
    },
    .reserved_pages = reserved_pages,
    .committed_pages = committed_pages,
  };

  return &it->parent;
}

} // namespace