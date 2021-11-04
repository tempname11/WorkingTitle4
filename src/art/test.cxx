#include <src/global.hxx>
#include <src/engine/system/artline/public.hxx>

#define DLL_EXPORT extern "C" __declspec(dllexport)

DLL_EXPORT DECL_DESCRIBE {
  return 42;
}