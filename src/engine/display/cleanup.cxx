#include <vulkan/vulkan.h>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/engine/helpers.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <src/engine/datum/probe_radiance.hxx>
#include <src/engine/datum/probe_irradiance.hxx>
#include <src/engine/datum/probe_attention.hxx>
#include <src/engine/step/probe_appoint.hxx>
#include <src/engine/step/probe_measure.hxx>
#include <src/engine/step/probe_collect.hxx>
#include <src/engine/step/indirect_light.hxx>
#include <backends/imgui_impl_vulkan.h>
#include "cleanup.hxx"

namespace engine::display {

void cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::Task> session_iteration_yarn_end,
  Use<engine::session::Data> session,
  Own<engine::display::Data> data
) {
  ZoneScoped;
  auto vulkan = &session->vulkan;
  auto core = &vulkan->core;

  for (auto image_view : data->gbuffer.channel0_views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }
  for (auto image_view : data->gbuffer.channel1_views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }
  for (auto image_view : data->gbuffer.channel2_views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }
  for (auto image_view : data->zbuffer.views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }
  for (auto image_view : data->lbuffer.views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }

  engine::helpers::deinit(
    &data->helpers,
    core
  );

  deinit_rendering_prepass(
    &data->prepass,
    core
  );

  deinit_rendering_gpass(
    &data->gpass,
    core
  );

  deinit_rendering_lpass(
    &data->lpass,
    core
  );

  step::probe_appoint::deinit_ddata(
    &data->probe_appoint,
    core
  );

  step::probe_measure::deinit_ddata(
    &data->probe_measure,
    core
  );

  step::probe_collect::deinit_ddata(
    &data->probe_collect,
    core
  );

  step::indirect_light::deinit_ddata(
    &data->indirect_light,
    core
  );

  deinit_rendering_finalpass(
    &data->finalpass,
    core
  );

  lib::gfx::multi_alloc::deinit(
    &data->multi_alloc,
    core->device,
    core->allocator
  );

  datum::probe_radiance::deinit_ddata(
    &data->probe_radiance,
    core
  );

  datum::probe_irradiance::deinit_ddata(
    &data->probe_irradiance,
    core
  );

  datum::probe_attention::deinit_ddata(
    &data->probe_attention,
    core
  );

  lib::gfx::allocator::deinit(
    &data->allocator_dedicated,
    core->device,
    core->allocator
  );

  lib::gfx::allocator::deinit(
    &data->allocator_shared,
    core->device,
    core->allocator
  );

  lib::gfx::allocator::deinit(
    &data->allocator_host,
    core->device,
    core->allocator
  );

  vkDestroyRenderPass(
    core->device,
    data->imgui_backend.render_pass,
    core->allocator
  );
  for (auto framebuffer : data->imgui_backend.framebuffers) {
    vkDestroyFramebuffer(
      core->device,
      framebuffer,
      core->allocator
    );
  }
  ImGui_ImplVulkan_Shutdown();
  for (auto &pool2 : data->command_pools) {
    for (auto pool1 : pool2.available) {
      vkDestroyCommandPool(
        core->device,
        pool1->pool,
        core->allocator
      );
      delete pool1;
    }
  }
  for (auto &pool : data->descriptor_pools) {
    vkDestroyDescriptorPool(
      core->device,
      pool.pool,
      core->allocator
    );
  }
  for (size_t i = 0; i < data->swapchain_description.image_count; i++) {
    vkDestroySemaphore(
      core->device,
      data->presentation.image_acquired[i],
      core->allocator
    );
    vkDestroySemaphore(
      core->device,
      data->presentation.image_rendered[i],
      core->allocator
    );
  }
  for (auto image_view : data->final_image.views) {
    vkDestroyImageView(
      core->device,
      image_view,
      core->allocator
    );
  }
  vkDestroySemaphore(
    core->device,
    data->graphics_finished_semaphore,
    core->allocator
  );
  vkDestroySemaphore(
    core->device,
    data->imgui_finished_semaphore,
    core->allocator
  );
  vkDestroySemaphore(
    core->device,
    data->frame_finished_semaphore,
    core->allocator
  );
  vkDestroySwapchainKHR(
    core->device,
    data->presentation.swapchain,
    core->allocator
  );
  delete data.ptr;
  ctx->changed_parents = {
    { .ptr = data.ptr, .children = {} }
  };
  lib::task::signal(ctx->runner, session_iteration_yarn_end.ptr);
}

} // namespace
