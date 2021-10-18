#include <mutex>
#include <src/global.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/startup.hxx>
#include <src/engine/loading/group.hxx>
#include <src/engine/session/setup.hxx>
#include "../scripting.inline.hxx"

// @Cleanup move to scripting
// v

Waitable allow_frames(
  Ctrl& ctrl,
  Ref<SessionData::FrameControl> fc,
  size_t count
) {
  auto lock = std::scoped_lock(fc->mutex);
  fc->allowed_count += count;
  if (fc->signal_allowed != nullptr) {
    lib::task::signal(ctrl.ctx->runner, fc->signal_allowed);
    fc->signal_allowed = nullptr;
  }

  assert(fc->signal_done == nullptr); // @Incomplete
  fc->signal_done = lib::task::create_yarn_signal();

  return ctrl.wait_for_signal(fc->signal_done);
}

void quit(
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
    }
  }
}

void interactive(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Ref<SessionData::FrameControl> fc,
  Own<SessionData::State> state,
  Own<SessionData::GLFW> glfw
) {
  state->ignore_glfw_events = false;
  glfwShowWindow(glfw->window);
  {
    auto lock = std::scoped_lock(fc->mutex);
    fc->allowed_count = SIZE_MAX / 2; // prevent overflow on +=
    if (fc->signal_allowed != nullptr) {
      lib::task::signal(ctx->runner, fc->signal_allowed);
    }
  }
}

void end(Ctrl& ctrl, SessionData *session) {
  if (1) {
    create(interactive, &session->frame_control, &session->state, &session->glfw);
  } else {
    create(quit, &session->frame_control, &session->glfw);
  }
}

// ^
// @Cleanup move to scripting

void update_camera(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<SessionData::State> state
) {
  state->debug_camera.position = glm::vec3(5.0f, 0.0f, 5.0f);
  state->debug_camera.lon_lat = glm::vec2(3.0f, 0.0f);
  lib::debug_camera::Input zero_input = {};
  lib::debug_camera::update(&state->debug_camera, &zero_input, 0.0);
}

void Ctrl::run() {
  SessionData *session = nullptr;
  create(engine::session::setup, &session);

  std::string path = "assets/sponza-fixed/index.grup";
  add(engine::loading::group::load(
    ctx,
    &path,
    session
  ));
  create(update_camera, &session->state);
  allow_frames(*this, &session->frame_control, 100);
  end(*this, session); // @Cleanup
}
