#ifdef WINDOWS
#include <Windows.h>
#include <cstdint>

namespace lib::array {

SYSTEM_INFO get_system_info() {
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  return system_info;
}
SYSTEM_INFO system_info = get_system_info();

size_t _virtual_page_size() {
  return system_info.dwPageSize;
}
void *_virtual_reserve(size_t size) {
  return VirtualAlloc(
    NULL,
    size,
    MEM_RESERVE,
    PAGE_READWRITE
  );
}

void _virtual_commit(void *mem, size_t size) {
  VirtualAlloc(
    mem,
    size,
    MEM_COMMIT,
    PAGE_READWRITE
  );
}

void _virtual_free(void *mem) {
  VirtualFree(mem, 0, MEM_RELEASE);
}

void _virtual_decommit(void *mem, size_t offset, size_t size) {
  VirtualFree((uint8_t *)mem + offset, size, MEM_DECOMMIT);
}

} // namespace

#endif
