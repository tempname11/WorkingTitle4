#ifdef WINDOWS
#include <cassert>
#include <Windows.h>

namespace lib::virtual_memory {

const char *tracy_pool_name = "Windows Virtual Memory";

SYSTEM_INFO get_system_info() {
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  return system_info;
}
SYSTEM_INFO system_info = get_system_info();

size_t get_page_size() {
  return system_info.dwPageSize;
}

void *reserve(size_t num_pages) {
  auto result = VirtualAlloc(
    NULL,
    num_pages * system_info.dwPageSize,
    MEM_RESERVE,
    PAGE_READWRITE
  );
  assert(result != nullptr);
  return result;
}

void commit(void *mem, size_t num_pages) {
  auto result = VirtualAlloc(
    mem,
    num_pages * system_info.dwPageSize,
    MEM_COMMIT,
    PAGE_READWRITE
  );
  assert(result != nullptr);
}

void free(void *mem) {
  auto result = VirtualFree(mem, 0, MEM_RELEASE);
  assert(result != 0);
}

} // namespace

#endif
