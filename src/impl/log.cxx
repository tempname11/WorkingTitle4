#include <Tracy.hpp>
#ifdef WINDOWS
  #include <Windows.h>
#endif
#include <string>

void _log(std::string &str) {
  TracyMessage(str.data(), str.size());
  #ifdef WINDOWS
    OutputDebugStringA(str.data());
  #else
    printf(str.data());
    printf("\n");
    fflush(stdout);
  #endif
}