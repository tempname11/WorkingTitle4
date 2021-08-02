#include "defer.hxx"
#include "session_iteration_try_rendering.hxx"
#include "session_iteration.hxx"

TASK_DECL {
  ZoneScoped;
  bool should_stop = glfwWindowShouldClose(data->glfw.window);
  if (should_stop) {
    task::signal(ctx->runner, session_yarn_end.ptr);
    return;
  } else {
    glfwWaitEvents();
    auto session_iteration_yarn_end = task::create_yarn_signal();
    auto task_try_rendering = task::create(
      session_iteration_try_rendering,
      session_iteration_yarn_end,
      data.ptr
    );
    auto task_repeat = defer(task::create(
      session_iteration,
      session_yarn_end.ptr,
      data.ptr
    ));
    task::inject(ctx->runner, {
      task_try_rendering,
      task_repeat.first
    }, {
      .new_dependencies = {
        { session_iteration_yarn_end, task_repeat.first }
      },
    });
  }
}
