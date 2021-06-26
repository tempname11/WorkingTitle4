#include <vulkan/vulkan.h>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <backends/imgui_impl_vulkan.h>
#include "task.hxx"

void deinit_example(
  RenderingData::Example *it,
  SessionData::Vulkan *vulkan
) {
  ZoneScoped;
  for (auto image_view : it->gbuffer.channel0_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : it->gbuffer.channel1_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : it->gbuffer.channel2_views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : it->zbuffer.views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  for (auto image_view : it->lbuffer.views) {
    vkDestroyImageView(
      vulkan->core.device,
      image_view,
      vulkan->core.allocator
    );
  }
  { ZoneScopedN(".prepass");
    for (auto framebuffer : it->prepass.framebuffers) {
      vkDestroyFramebuffer(
        vulkan->core.device,
        framebuffer,
        vulkan->core.allocator
      );
    }
    vkDestroyRenderPass(
      vulkan->core.device,
      it->prepass.render_pass,
      vulkan->core.allocator
    );
    vkDestroyPipeline(
      vulkan->core.device,
      it->prepass.pipeline,
      vulkan->core.allocator
    );
  }
  { ZoneScopedN(".gpass");
    for (auto framebuffer : it->gpass.framebuffers) {
      vkDestroyFramebuffer(
        vulkan->core.device,
        framebuffer,
        vulkan->core.allocator
      );
    }
    vkDestroyRenderPass(
      vulkan->core.device,
      it->gpass.render_pass,
      vulkan->core.allocator
    );
    vkDestroyPipeline(
      vulkan->core.device,
      it->gpass.pipeline,
      vulkan->core.allocator
    );
  }
  {
    ZoneScopedN(".lpass");
    for (auto framebuffer : it->lpass.framebuffers) {
      vkDestroyFramebuffer(
        vulkan->core.device,
        framebuffer,
        vulkan->core.allocator
      );
    }
    vkDestroyRenderPass(
      vulkan->core.device,
      it->lpass.render_pass,
      vulkan->core.allocator
    );
    vkDestroyPipeline(
      vulkan->core.device,
      it->lpass.pipeline_sun,
      vulkan->core.allocator
    );
  }
}

void rendering_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Some<SessionData> session,
  usage::Full<RenderingData> data
) {
  ZoneScoped;
  auto vulkan = &session->vulkan;
  deinit_example(
    &data->example,
    vulkan
  );
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
    data->example_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->imgui_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->frame_rendered_semaphore,
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
