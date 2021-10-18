#include <src/engine/startup.hxx>

#ifdef WINDOWS
  int WinMain() {
    engine::startup(nullptr);
    return 0;
  }
#else
  int main() {
    engine::startup(nullptr);
    return 0;
  }
#endif
