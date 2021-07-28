#include <Tracy.hpp>
#ifdef WINDOWS
  #include <Windows.h>
#endif
#include <string>

void _log(std::string &str) {
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