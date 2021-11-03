#include <mutex>
#include <src/global.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/startup.hxx>
#include <src/engine/system/grup/group.hxx>
#include <src/engine/session/setup.hxx>
#undef WINDOWS
#include "session.inline.hxx"

typedef int (TestFn)();

void update_stuff(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<engine::session::Data::State> state
) {
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0, 0.0);
}

void runtest();

void CtrlSession::run() {
  runtest();
  {
    std::string path = "assets/vox/medieval_city_2/index.grup";
    task(engine::system::grup::group::load(
      ctx,
      &path,
      session
    ));
    task(update_stuff, &session->state);
  }
}

MAIN_MACRO(CtrlSession);

#undef APIENTRY
#include <windows.h>

void runtest() {
  HINSTANCE h_lib = LoadLibrary("testlib.dll");
  assert(h_lib != nullptr);

  auto test_fn = (TestFn *) GetProcAddress(h_lib, "test");
  assert(test_fn != nullptr);

  test_fn();

  auto result = FreeLibrary(h_lib);
  assert(result != 0);
}

int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
) {
  return main();
}
