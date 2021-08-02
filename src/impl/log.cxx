#include <Tracy.hpp>
#ifdef WINDOWS
  #include <Windows.h>
#endif
#include <string>
#include <mutex>

std::mutex log_mutex;

void _log(std::string &str) {
  std::scoped_lock lock(log_mutex);
  TracyMessage(str.data(), str.size());
  #ifdef WINDOWS
    OutputDebugString(str.data());

    OutputDebugString("\n");
    // quick fix for missing newline.
    // would be better to do it in one call.
  #else
    printf(str.data());
    printf("\n");
    fflush(stdout);
  #endif
}