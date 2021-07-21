#include <src/global.hxx>

namespace lib::array {

size_t _virtual_page_size();
void *_virtual_reserve(size_t size);
void _virtual_commit(void *mem, size_t size);
void _virtual_decommit(void *mem, size_t offset, size_t size);
void _virtual_free(void *mem);

template<typename T>
struct Array {
  size_t count;
  size_t total_reserved_size;
  size_t total_committed_size;
  T data[];
};

template<typename T>
Array<T> *create(size_t max_count) {
  auto page_size = _virtual_page_size();
  size_t needed_reserved_size = sizeof(Array<T>) + max_count * sizeof(T);
  size_t needed_committed_size = sizeof(Array<T>);
  size_t total_reserved_size = ((needed_reserved_size + page_size - 1) / page_size) * page_size;
  size_t total_committed_size = ((needed_committed_size + page_size - 1) / page_size) * page_size;
  auto mem = _virtual_reserve(total_reserved_size);
  _virtual_commit(mem, total_committed_size);
  auto self = (Array<T> *) mem;
  self->count = 0;
  self->total_committed_size = total_committed_size;
  self->total_reserved_size = total_reserved_size;
  TracyAlloc(self, self->total_committed_size);
  return self;
}

template<typename T>
T* add_back(Array<T> *self, size_t amount) {
  self->count += amount;
  size_t needed_committed_size = sizeof(Array<T>) + self->count * sizeof(T);
  if (self->total_committed_size < needed_committed_size) {
    auto page_size = _virtual_page_size();
    size_t new_total_committed_size = ((needed_committed_size + page_size - 1) / page_size) * page_size;
    assert(new_total_committed_size <= self->total_reserved_size);
    _virtual_commit(self, new_total_committed_size);
    self->total_committed_size = new_total_committed_size;
    TracyFree(self);
    TracyAlloc(self, self->total_committed_size);
  }
  return &self->data[self->count - amount];
}

template<typename T>
void remove_back(Array<T> *self, size_t amount) {
  assert(amount <= self->count);
  self->count -= amount;
  auto page_size = _virtual_page_size();
  size_t needed_committed_size = sizeof(Array<T>) + self->count * sizeof(T);
  size_t new_total_committed_size = ((needed_committed_size + page_size - 1) / page_size) * page_size;
  size_t decommit_size = self->total_committed_size - new_total_committed_size;
  _virtual_decommit(self, new_total_committed_size, decommit_size);
  self->total_committed_size = new_total_committed_size;
  TracyFree(self);
  TracyAlloc(self, self->total_committed_size);
}

template<typename T>
void destroy(Array<T> *self) {
  _virtual_free(self);
  TracyFree(self);
}

} // namespace