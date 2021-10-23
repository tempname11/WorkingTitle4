#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <src/engine/rendering/pass/probe_measure.hxx>
#include <src/engine/rendering/pass/probe_collect.hxx>
#include <src/engine/rendering/pass/indirect_light.hxx>
#include "cleanup.hxx"

namespace engine::session {

void cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data> session
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
    auto core = &it->core;

    deinit_session_prepass(&it->prepass, core);

    deinit_session_gpass(&it->gpass, core);

    deinit_session_lpass(&it->lpass, core);

    deinit_session_finalpass(&it->finalpass, core);

    rendering::pass::probe_measure::deinit_sdata(
      &it->pass_probe_measure,
      core
    );

    rendering::pass::probe_collect::deinit_sdata(
      &it->pass_probe_collect,
      core
    );

    rendering::pass::indirect_light::deinit_sdata(
      &it->pass_indirect_light,
      core
    );

    { ZoneScopedN(".multi_alloc");
      lib::gfx::multi_alloc::deinit(
        &it->multi_alloc,
        it->core.device,
        it->core.allocator
      );
    }

    { ZoneScopedN(".uploader");
      engine::uploader::deinit(
        &it->uploader,
        it->core.device,
        it->core.allocator
      );
    }

    { ZoneScopedN(".blas_storage");
      engine::blas_storage::deinit(
        &it->blas_storage,
        &it->core
      );
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

} // namespace
