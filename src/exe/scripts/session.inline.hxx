#pragma once
#include <semaphore>
#include <src/lib/task.hxx>
#include "common.inline.hxx"

void Ctrl::run() {} // ignore

void _quit(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Ref<SessionData::FrameControl> fc,
  Own<SessionData::GLFW> glfw
) {
  glfwSetWindowShouldClose(glfw->window, 1);
  {
    auto lock = std::scoped_lock(fc->mutex);
    fc->allowed_count = 1;
    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }
  }
}

void _interactive(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Ref<SessionData::FrameControl> fc,
  Own<SessionData::State> state,
  Own<SessionData::GLFW> glfw
) {
  state->ignore_glfw_events = false;
  //glfwShowWindow(glfw->window);
  {
    auto lock = std::scoped_lock(fc->mutex);
    fc->allowed_count = SIZE_MAX / 2; // prevent overflow on +=
    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }
  }
}

struct CtrlSession : Ctrl {
  SessionData *session;

  void screenshot_next_frame(std::string &path) {
    auto fc = &session->frame_control;
    auto lock = std::scoped_lock(fc->mutex);
    fc->directives.should_capture_screenshot = true;
    fc->directives.screenshot_path = path;
  }

  Waitable advance_frames(size_t count) {
    auto fc = &session->frame_control;
    auto lock = std::scoped_lock(fc->mutex);
    fc->allowed_count += count;

    assert(fc->signal_done == nullptr); // @Incomplete
    fc->signal_done = lib::task::create_yarn_signal();

    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
      fc->signal_allowed = nullptr;
    }

    return wait_for_signal(fc->signal_done);
  }

  void run();
  void run_decorated() {
    task(engine::session::setup, &session);
    run();
    if (getenv("ENGINE_ENV_SILENT")) {
      task(_quit, &session->frame_control, &session->glfw);
    } else {
      task(_interactive, &session->frame_control, &session->state, &session->glfw);
    }
  }
};
