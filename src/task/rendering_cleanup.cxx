#include <vulkan/vulkan.h>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <backends/imgui_impl_vulkan.h>
#include "task.hxx"

void rendering_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Some<SessionData> session,
  usage::Full<RenderingData> data
) {
  ZoneScoped;
  auto vulkan = &session->vulkan;
  for (auto image_view : data->gbuffer.channel0_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : data->gbuffer.channel1_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : data->gbuffer.channel2_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : data->zbuffer.views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : data->lbuffer.views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }

  deinit_rendering_prepass(
    &data->prepass,
    &vulkan->core
  );
  deinit_rendering_gpass(
    &data->gpass,
    &vulkan->core
  );

  {
    ZoneScopedN(".lpass");
    for (auto framebuffer : data->lpass.framebuffers) {
      vkDestroyFramebuffer(
        vulkan->core.device,
        framebuffer,
        vulkan->core.allocator
      );
    }
    vkDestroyRenderPass(
      vulkan->core.device,
      data->lpass.render_pass,
      vulkan->core.allocator
    );
    vkDestroyPipeline(
      vulkan->core.device,
      data->lpass.pipeline_sun,
      vulkan->core.allocator
    );
  }
  vkDestroyDescriptorPool(
    vulkan->core.device,
    data->common_descriptor_pool,
    vulkan->core.allocator
  );
  lib::gfx::multi_alloc::deinit(
    &data->multi_alloc,
    vulkan->core.device,
    vulkan->core.allocator
  );
  vkDestroyRenderPass(
    vulkan->core.device,
    data->imgui_backend.render_pass,
    vulkan->core.allocator
  );
  for (auto framebuffer : data->imgui_backend.framebuffers) {
    vkDestroyFramebuffer(
      vulkan->core.device,
      framebuffer,
      vulkan->core.allocator
    );
  }
  ImGui_ImplVulkan_Shutdown();
  for (auto &pool2 : data->command_pools) {
    for (auto pool : pool2.pools) {
      vkDestroyCommandPool(
        vulkan->core.device,
        pool,
        vulkan->core.allocator
      );
    }
  }
  for (size_t i = 0; i < data->swapchain_description.image_count; i++) {
    vkDestroySemaphore(
      vulkan->core.device,
      data->presentation.image_acquired[i],
      vulkan->core.allocator
    );
    vkDestroySemaphore(
      vulkan->core.device,
      data->presentation.image_rendered[i],
      vulkan->core.allocator
    );
  }
  for (auto image_view : data->final_image.views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  vkDestroySemaphore(
    vulkan->core.device,
    data->graphics_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->imgui_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->frame_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySwapchainKHR(
    vulkan->core.device,
    data->presentation.swapchain,
    vulkan->core.allocator
  );
  delete data.ptr;
  ctx->changed_parents = {
    { .ptr = data.ptr, .children = {} }
  };
  task::signal(ctx->runner, session_iteration_yarn_end.ptr);
}
