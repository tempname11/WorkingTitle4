#include "task.hxx"

// #define ENGINE_DEBUG_ARTIFICIAL_DELAY 33ms

void rendering_frame(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<task::Task> rendering_yarn_end,
  usage::None<SessionData> session,
  usage::None<RenderingData> data,
  usage::Some<SessionData::GLFW> glfw,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Full<RenderingData::FrameInfo> latest_frame,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::InflightGPU> inflight_gpu
) {
  ZoneScopedC(0xFF0000);
  FrameMark;
  #ifdef ENGINE_DEBUG_ARTIFICIAL_DELAY
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(ENGINE_DEBUG_ARTIFICIAL_DELAY);
    }
  #endif
  bool presentation_has_failed;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_has_failed = presentation_failure_state->failure;
  }
  bool should_stop = glfwWindowShouldClose(glfw->window);
  if (should_stop || presentation_has_failed) {
    std::scoped_lock lock(inflight_gpu->mutex);
    auto task_has_finished = task::create(rendering_has_finished, rendering_yarn_end.ptr);
    task::Auxiliary aux;
    for (auto signal : inflight_gpu->signals) {
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
  auto frame_info = new RenderingData::FrameInfo(*latest_frame);
  auto compose_data = new ComposeData;
  auto example_data = new ExampleData;
  auto imgui_data = new ImguiData;
  auto update_data = new UpdateData;
  auto frame_tasks = new std::vector<task::Task *>({
    task::create(
      rendering_frame_handle_window_events,
      &session->glfw,
      &session->state,
      update_data
    ),
    task::create(
      rendering_frame_update,
      update_data,
      frame_info,
      &session->state
    ),
    task::create(
      rendering_frame_setup_gpu_signal,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &data->frame_rendered_semaphore,
      &data->inflight_gpu,
      frame_info
    ),
    task::create(
      rendering_frame_reset_pools,
      &session->vulkan.core,
      &data->command_pools,
      frame_info
    ),
    task::create(
      rendering_frame_acquire,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info
    ),
    task::create(
      rendering_frame_example_prepare_uniforms,
      &session->vulkan.core,
      &data->swapchain_description,
      frame_info,
      &session->state,
      &data->example
    ),
    task::create(
      rendering_frame_example_render,
      &session->vulkan.core,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->final_image,
      &data->example,
      &session->vulkan.example,
      example_data
    ),
    task::create(
      rendering_frame_example_submit,
      &session->vulkan.queue_work,
      &data->example_finished_semaphore,
      frame_info,
      example_data
    ),
    task::create(
      rendering_frame_imgui_new_frame,
      &session->imgui_context,
      &data->imgui_backend
    ),
    task::create(
      rendering_frame_imgui_populate,
      &session->imgui_context,
      &session->state
    ),
    task::create(
      rendering_frame_imgui_render,
      &session->vulkan.core,
      &session->imgui_context,
      &data->imgui_backend,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      imgui_data
    ),
    task::create(
      rendering_frame_imgui_submit,
      &session->vulkan.queue_work,
      &data->example_finished_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      imgui_data
    ),
    task::create(
      rendering_frame_compose_render,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->final_image,
      compose_data
    ),
    task::create(
      rendering_frame_compose_submit,
      &session->vulkan.queue_work,
      &data->presentation,
      &data->presentation_failure_state,
      &data->frame_rendered_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      compose_data
    ),
    task::create(
      rendering_frame_present,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info,
      &session->vulkan.queue_present
    ),
    task::create(
      rendering_frame_cleanup,
      frame_info
    ),
    task::create(
      rendering_frame,
      rendering_yarn_end.ptr,
      session.ptr,
      data.ptr,
      glfw.ptr,
      presentation_failure_state.ptr,
      latest_frame.ptr,
      swapchain_description.ptr,
      inflight_gpu.ptr
    ),
  });
  auto task_defer = task::create(defer_many, frame_tasks);
  { // inject under inflight mutex
    std::scoped_lock lock(inflight_gpu->mutex);
    auto signal = inflight_gpu->signals[latest_frame->inflight_index];
    task::inject(ctx->runner, {
      task_defer,
    }, {
      .new_dependencies = { { signal, task_defer } },
    });
  }
}
