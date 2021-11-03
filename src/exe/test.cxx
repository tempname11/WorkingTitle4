#include <src/global.hxx>

extern "C" __declspec(dllexport) int __cdecl test() {
  LOG("test!");
  return 0;
}