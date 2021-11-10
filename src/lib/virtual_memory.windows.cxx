#ifdef WINDOWS
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
  return VirtualAlloc(
    NULL,
    num_pages * system_info.dwPageSize,
    MEM_RESERVE,
    PAGE_READWRITE
  );
}

void commit(void *mem, size_t num_pages) {
  VirtualAlloc(
    mem,
    num_pages * system_info.dwPageSize,
    MEM_COMMIT,
    PAGE_READWRITE
  );
}

void free(void *mem) {
  VirtualFree(mem, 0, MEM_RELEASE);
}

} // namespace

#endif
