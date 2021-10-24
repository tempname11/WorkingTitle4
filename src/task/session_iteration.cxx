#include <src/lib/defer.hxx>
#include "session_iteration_try_rendering.hxx"
#include "session_iteration.hxx"

void session_iteration(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<engine::session::Data> session
) {
  ZoneScoped;
  bool should_stop = glfwWindowShouldClose(session->glfw.window);
  if (should_stop) {
    for (auto &item : session->groups.items) {
      auto final = lib::lifetime::deref(&item.second.lifetime, ctx->runner);
      // assert(final); @Think: was this actually necessary?
    }
    lib::lifetime::deref(&session->lifetime, ctx->runner);
    return;
  } else {
    glfwWaitEvents();
    auto session_iteration_yarn_end = lib::task::create_yarn_signal();
    auto task_try_rendering = lib::task::create(
      session_iteration_try_rendering,
      session_iteration_yarn_end,
      session.ptr
    );
    auto task_repeat = lib::defer(lib::task::create(
      session_iteration,
      session.ptr
    ));
    lib::task::inject(ctx->runner, {
      task_try_rendering,
      task_repeat.first
    }, {
      .new_dependencies = {
        { session_iteration_yarn_end, task_repeat.first }
      },
    });
  }
}
