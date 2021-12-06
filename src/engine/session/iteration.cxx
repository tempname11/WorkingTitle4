#include <src/lib/defer.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include <src/engine/display/setup.hxx>
#include <src/engine/system/grup/data.hxx>
#include "iteration.hxx"

namespace engine::session {

void _try_rendering(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<lib::Task> session_iteration_yarn_end,
  Own<engine::session::Data> session
) {
  ZoneScoped;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  { // get capabilities
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      session->vulkan->physical_device,
      session->vulkan->window_surface,
      &surface_capabilities
    );
    assert(result == VK_SUCCESS);
  }

  if (
    surface_capabilities.currentExtent.width == 0 ||
    surface_capabilities.currentExtent.height == 0
  ) {
    lib::task::signal(ctx->runner, session_iteration_yarn_end.ptr);
    return;
  }

  engine::display::setup(
    ctx,
    session_iteration_yarn_end,
    session,
    &surface_capabilities
  );
}

void iteration(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Use<engine::session::Data> session
) {
  ZoneScoped;
  bool should_stop = glfwWindowShouldClose(session->glfw.window);
  if (should_stop) {
    for (auto &item : session->grup.groups->items) {
      auto final = lib::lifetime::deref(&item.second.lifetime, ctx->runner);
      // assert(final); @Think: was this actually necessary?
    }
    lib::lifetime::deref(&session->lifetime, ctx->runner);
    return;
  } else {
    glfwWaitEvents();
    auto session_iteration_yarn_end = lib::task::create_yarn_signal();
    auto task_try_rendering = lib::task::create(
      _try_rendering,
      session_iteration_yarn_end,
      session.ptr
    );
    auto task_repeat = lib::defer(lib::task::create(
      iteration,
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

} // namespace
