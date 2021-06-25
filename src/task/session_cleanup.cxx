#include <imgui.h>
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
    { ZoneScopedN(".example");
      vkDestroyDescriptorSetLayout(
        it->core.device,
        it->example.gpass.descriptor_set_layout,
        it->core.allocator
      );
      vkDestroyPipelineLayout(
        it->core.device,
        it->example.gpass.pipeline_layout,
        it->core.allocator
      );
      vkDestroyDescriptorSetLayout(
        it->core.device,
        it->example.lpass.descriptor_set_layout,
        it->core.allocator
      );
      vkDestroyPipelineLayout(
        it->core.device,
        it->example.lpass.pipeline_layout,
        it->core.allocator
      );
      vkDestroyPipeline(
        it->core.device,
        it->example.finalpass.pipeline,
        it->core.allocator
      );
      vkDestroyPipelineLayout(
        it->core.device,
        it->example.finalpass.pipeline_layout,
        it->core.allocator
      );
      vkDestroyDescriptorSetLayout(
        it->core.device,
        it->example.finalpass.descriptor_set_layout,
        it->core.allocator
      );
      vkDestroySampler(
        it->core.device,
        it->example.finalpass.sampler_lbuffer,
        it->core.allocator
      );
    }
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
    vkDestroyDescriptorPool(
      it->core.device,
      it->common_descriptor_pool,
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
