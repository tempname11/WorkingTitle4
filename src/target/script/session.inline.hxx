#pragma once
#include <semaphore>
#include <src/lib/task.hxx>
#include <src/engine/session/public.hxx>
#include "common.inline.hxx"

void Ctrl::run() {} // ignore

void _quit(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Ref<engine::session::Data::FrameControl> fc,
  Own<engine::session::Data::GLFW> glfw
) {
  glfwSetWindowShouldClose(glfw->window, 1);
  {
    lib::mutex::lock(&fc->mutex);

    fc->allowed_count = 1;
    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }

    lib::mutex::unlock(&fc->mutex);
  }
}

void _interactive(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Ref<engine::session::Data::FrameControl> fc,
  Own<engine::session::Data::State> state,
  Own<engine::session::Data::GLFW> glfw
) {
  state->ignore_glfw_events = false;
  //glfwShowWindow(glfw->window);
  {
    lib::mutex::lock(&fc->mutex);

    fc->allowed_count = SIZE_MAX / 2; // prevent overflow on +=
    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }

    lib::mutex::unlock(&fc->mutex);
  }
}

struct CtrlSession : Ctrl {
  engine::session::Data *session;

  void screenshot_next_frame(std::string &path) {
    auto fc = session->frame_control;
    lib::mutex::lock(&fc->mutex);

    fc->directives.should_capture_screenshot = true;
    fc->directives.screenshot_path = path;

    lib::mutex::unlock(&fc->mutex);
  }

  Waitable advance_frames(size_t count) {
    auto fc = session->frame_control;
    lib::mutex::lock(&fc->mutex);

    fc->allowed_count += count;

    assert(fc->signal_done == nullptr); // @Incomplete
    fc->signal_done = lib::task::create_yarn_signal();

    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }

    lib::mutex::unlock(&fc->mutex);
    return wait_for_signal(fc->signal_done);
  }

  void run();
  void run_decorated() {
    task(engine::session::setup, &session);
    run();
    if (getenv("ENGINE_ENV_SILENT")) {
      task(_quit, session->frame_control, &session->glfw);
    } else {
      task(_interactive, session->frame_control, &session->state, &session->glfw);
    }
  }
};
