#include "defer.hxx"
#include "frame_acquire.hxx"
#include "frame_cleanup.hxx"
#include "frame_compose_render.hxx"
#include "frame_compose_submit.hxx"
#include "frame_generate_render_list.hxx"
#include "frame_graphics_render.hxx"
#include "frame_graphics_submit.hxx"
#include "frame_handle_window_events.hxx"
#include "frame_imgui_new_frame.hxx"
#include "frame_imgui_populate.hxx"
#include "frame_imgui_render.hxx"
#include "frame_imgui_submit.hxx"
#include "frame_loading_dynamic.hxx"
#include "frame_prepare_uniforms.hxx"
#include "frame_present.hxx"
#include "frame_reset_pools.hxx"
#include "frame_setup_gpu_signal.hxx"
#include "frame_update.hxx"
#include "rendering_has_finished.hxx"
#include "rendering_frame.hxx"

#define TRACY_ARTIFICIAL_DELAY 20ms

TASK_DECL {
  ZoneScopedC(0xFF0000);
  FrameMark;
  if (TracyIsConnected) {
    ZoneScopedN("artificial_delay");
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(TRACY_ARTIFICIAL_DELAY);
  }
  bool presentation_has_failed;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_has_failed = presentation_failure_state->failure;
  }
  bool should_stop = glfwWindowShouldClose(glfw->window);
  if (should_stop || presentation_has_failed) {
    std::scoped_lock lock(session->inflight_gpu.mutex);
    auto task_has_finished = task::create(rendering_has_finished, rendering_yarn_end.ptr);
    task::Auxiliary aux;
    for (auto signal : session->inflight_gpu.signals) {
      aux.new_dependencies.push_back({ signal, task_has_finished });
    }
    task::inject(ctx->runner, { task_has_finished }, std::move(aux));
    return;
  }
  { // timing
    using namespace std::chrono_literals;
    uint64_t new_timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch() / 1ns;
    if (new_timestamp_ns > latest_frame->timestamp_ns) {
      latest_frame->elapsed_ns = new_timestamp_ns - latest_frame->timestamp_ns;
    } else {
      latest_frame->elapsed_ns = 0;
    }
    latest_frame->timestamp_ns = new_timestamp_ns;
  }
  latest_frame->number++;
  latest_frame->timeline_semaphore_value = latest_frame->number + 1;
  latest_frame->inflight_index = (
    latest_frame->number % swapchain_description->image_count
  );

  // @Improvement: should put FrameInfo inside FrameData.
  auto frame_info = new RenderingData::FrameInfo(*latest_frame);
  auto frame_data = new engine::misc::FrameData {};

  auto task_setup_gpu_signal = defer(
    task::create(
      frame_setup_gpu_signal,
      session.ptr,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &data->frame_finished_semaphore,
      frame_info
    )
  );
  auto frame_tasks = new std::vector<task::Task *>({
    task::create(
      frame_handle_window_events,
      &session->glfw,
      &session->state,
      &frame_data->update_data
    ),
    task::create(
      frame_update,
      &frame_data->update_data,
      frame_info,
      &session->state
    ),
    task::create(
      frame_reset_pools,
      &session->vulkan.core,
      &data->command_pools,
      &data->descriptor_pools,
      frame_info
    ),
    task::create(
      frame_acquire,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info
    ),
    task::create(
      frame_prepare_uniforms,
      &session->vulkan.core,
      &data->swapchain_description,
      frame_info,
      &session->state,
      &data->common,
      &data->gpass,
      &data->lpass
    ),
    task::create(
      frame_generate_render_list,
      session.ptr,
      &session->scene,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      &frame_data->render_list
    ),
    task::create(
      frame_graphics_render,
      &session->vulkan.core,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->descriptor_pools,
      &data->prepass,
      &data->gpass,
      &data->lpass,
      &data->finalpass,
      &data->zbuffer,
      &data->gbuffer,
      &data->lbuffer,
      &data->final_image,
      &session->vulkan.prepass,
      &session->vulkan.gpass,
      &session->vulkan.lpass,
      &session->vulkan.finalpass,
      &session->vulkan.fullscreen_quad,
      &frame_data->render_list,
      &frame_data->graphics_data
    ),
    task::create(
      frame_graphics_submit,
      &session->vulkan.queue_work,
      &data->graphics_finished_semaphore,
      frame_info,
      &frame_data->graphics_data
    ),
    task::create(
      frame_imgui_new_frame,
      &session->imgui_context,
      &data->imgui_backend,
      &session->glfw
    ),
    task::create(
      frame_imgui_populate,
      session.ptr,
      &session->imgui_context,
      &frame_data->imgui_reactions,
      &session->meta_meshes,
      &session->state
    ),
    task::create(
      frame_imgui_render,
      &session->vulkan.core,
      &session->imgui_context,
      &data->imgui_backend,
      &session->glfw,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &frame_data->imgui_data
    ),
    task::create(
      frame_imgui_submit,
      &session->vulkan.queue_work,
      &data->graphics_finished_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      &frame_data->imgui_data
    ),
    task::create(
      frame_compose_render,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->final_image,
      &frame_data->compose_data
    ),
    task::create(
      frame_compose_submit,
      &session->vulkan.queue_work,
      &data->presentation,
      &data->presentation_failure_state,
      &data->frame_finished_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      &frame_data->compose_data
    ),
    task::create(
      frame_present,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info,
      &session->vulkan.queue_present
    ),
    task::create(
      frame_loading_dynamic,
      session.ptr,
      &session->guid_counter,
      &session->meta_meshes,
      &frame_data->imgui_reactions
    ),
    task::create(
      frame_cleanup,
      frame_info,
      frame_data
    ),
    task::create(
      rendering_frame,
      rendering_yarn_end.ptr,
      session.ptr,
      data.ptr,
      glfw.ptr,
      presentation_failure_state.ptr,
      latest_frame.ptr,
      swapchain_description.ptr
    ),
  });
  auto task_many = defer_many(frame_tasks);
  { // inject under inflight mutex
    std::scoped_lock lock(session->inflight_gpu.mutex);
    auto signal = session->inflight_gpu.signals[latest_frame->inflight_index];
    task::inject(ctx->runner, {
      task_setup_gpu_signal.first,
      task_many.first,
    }, {
      .new_dependencies = {
        { signal, task_setup_gpu_signal.first },
        { task_setup_gpu_signal.second, task_many.first },
      },
      .changed_parents = {
        {
          .ptr = frame_data,
          .children = {
            &frame_data->compose_data,
            &frame_data->graphics_data,
            &frame_data->imgui_data,
            &frame_data->update_data,
            &frame_data->render_list,
            &frame_data->imgui_reactions,
          },
        },
      },
    });
  }
}
