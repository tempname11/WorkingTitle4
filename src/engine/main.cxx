#include <GLFW/glfw3.h>
#include <src/lib/task.hxx>
#include <src/global.hxx>

struct Session {
  lib::task::AccessMarker m;
  struct MainLoop {
    lib::task::AccessMarker m;
    GLFWwindow *window;
    lib::task::Signal stop_signal;
  } main_loop;
};

void task_session_cleanup(
  lib::task::Context *ctx,
  lib::task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  lib::task::Exclusive<Session> session
) {
  ZoneScoped;
  glfwDestroyWindow(session->main_loop.window);
  glfwTerminate();
}

void task_session_main_loop_iteration(
  lib::task::Context *ctx,
  lib::task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  lib::task::Exclusive<Session::MainLoop> data
) {
  ZoneScoped;
  glfwWaitEvents();
  bool should_stop = glfwWindowShouldClose(data->window);
  if (should_stop) {
    lib::task::signal(ctx->runner, data->stop_signal);
  } else {
    lib::task::inject(ctx->runner, {
      lib::task::describe(task_session_main_loop_iteration, data.ptr),
    });
  }
}

void task_session_main_loop(
  lib::task::Context *ctx,
  lib::task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  lib::task::Exclusive<Session> session
) {
  ZoneScoped;
  lib::task::inject(ctx->runner, {
    lib::task::describe(task_session_main_loop_iteration, &session->main_loop),
  });
  ctx->signals = { session->main_loop.stop_signal };
}

const int DEFAULT_WINDOW_WIDTH = 1280;
const int DEFAULT_WINDOW_HEIGHT = 720;

void task_session(
  lib::task::Context *ctx,
  lib::task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>
) {
  ZoneScoped;
  auto session = new Session;
  session->main_loop.stop_signal = lib::task::create_signal();
  GLFWwindow *window;
  { // glfw stuff
    {
      bool success = glfwInit();
      assert(success == GLFW_TRUE);
    }
    glfwSetErrorCallback([](int error_code, const char* description) {
      LOG("GLFW error, code: {}, description: {}", error_code, description);
    });
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // for Vulkan
    GLFWmonitor *monitor = nullptr;
    int width = DEFAULT_WINDOW_WIDTH;
    int height = DEFAULT_WINDOW_HEIGHT;
    #if 0
      // duplicates code in process_settings
      monitor = glfwGetPrimaryMonitor();
      auto mode = glfwGetVideoMode(monitor);
      // hints for borderless fullscreen
      glfwWindowHint(GLFW_RED_BITS, mode->redBits);
      glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
      glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
      glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
      width = mode->width;
      height = mode->height;
    #endif
    window = glfwCreateWindow(width, height, "WorkingTitle", monitor, nullptr);
    assert(window != nullptr);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  }
  session->main_loop.window = window;
  // session is fully initialized at this point

  ctx->subtasks = {
    lib::task::describe(task_session_main_loop, session),
    lib::task::describe(task_session_cleanup, session),
  };
}

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
);

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

int main() {
  auto runner = lib::task::create_runner(QUEUE_COUNT);
  lib::task::inject(runner, {
    lib::task::describe(task_session),
  });
  #ifndef NDEBUG
    auto num_threads = 4u;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  std::vector worker_access_flags(num_threads - 1, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
  lib::task::run(runner, std::move(worker_access_flags), QUEUE_ACCESS_FLAGS_MAIN_THREAD);
  lib::task::discard_runner(runner);
  #ifdef TRACY_NO_EXIT
    printf("Waiting for profiler...\n");
    fflush(stdout);
  #endif
  return 0;
}

#ifdef WINDOWS
  int WinMain() {
    return main();
  }
#endif