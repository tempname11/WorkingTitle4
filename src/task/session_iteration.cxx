#include "task.hxx"

void session_iteration(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<task::Task> session_yarn_end,
  usage::Some<SessionData> data
) {
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
    auto task_repeat = task::create(defer, task::create(
      session_iteration,
      session_yarn_end.ptr,
      data.ptr
    ));
    task::inject(ctx->runner, {
      task_try_rendering,
      task_repeat
    }, {
      .new_dependencies = { { session_iteration_yarn_end, task_repeat } },
    });
  }
}
