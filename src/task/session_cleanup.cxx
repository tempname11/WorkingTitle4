#include <imgui.h>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <backends/imgui_impl_glfw.h>
#include "task.hxx"

void session_cleanup(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData> session
) {
  ZoneScoped;
  { ZoneScopedN(".gpu_signal_support");
    lib::gpu_signal::deinit_support(
      &session->gpu_signal_support,
      session->vulkan.core.device,
      session->vulkan.core.allocator
    );
  }
  { ZoneScopedN(".vulkan");
    auto it = &session->vulkan;

    deinit_session_prepass(
      &session->vulkan.prepass,
      &session->vulkan.core
    );

    deinit_session_gpass(
      &session->vulkan.gpass,
      &session->vulkan.core
    );

    deinit_session_lpass(
      &session->vulkan.lpass,
      &session->vulkan.core
    );

    deinit_session_finalpass(
      &session->vulkan.finalpass,
      &session->vulkan.core
    );

    { ZoneScopedN(".multi_alloc");
      lib::gfx::multi_alloc::deinit(&it->multi_alloc, it->core.device, it->core.allocator);
    }

    { ZoneScopedN(".core.tracy_context");
      TracyVkDestroy(it->core.tracy_context);
    }

    vkDestroyCommandPool(
      it->core.device,
      it->tracy_setup_command_pool,
      it->core.allocator
    );

    auto _vkDestroyDebugUtilsMessengerEXT = 
      (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        it->instance,
        "vkDestroyDebugUtilsMessengerEXT"
      );

    _vkDestroyDebugUtilsMessengerEXT(
      it->instance,
      it->debug_messenger,
      it->core.allocator
    );

    vkDestroyDevice(it->core.device, it->core.allocator);
    vkDestroySurfaceKHR(it->instance, it->window_surface, it->core.allocator);
    vkDestroyInstance(it->instance, it->core.allocator);
    assert(it->core.allocator == nullptr); // move along, nothing to destroy
  }

  { ZoneScopedN(".imgui_context");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  } 

  { ZoneScopedN(".glfw");
    glfwDestroyWindow(session->glfw.window);
    glfwTerminate();
  }

  delete session.ptr;
  ctx->changed_parents = {
    { .ptr = session.ptr, .children = {} },
    { .ptr = &session->vulkan, .children = {} },
  };
}
