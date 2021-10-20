#include <src/task/defer.hxx>
#include <src/task/rendering_has_finished.hxx>
#include "acquire.hxx"
#include "cleanup.hxx"
#include "compose_render.hxx"
#include "compose_submit.hxx"
#include "generate_render_list.hxx"
#include "graphics_render.hxx"
#include "graphics_submit.hxx"
#include "handle_window_events.hxx"
#include "imgui_new_frame.hxx"
#include "imgui_populate.hxx"
#include "imgui_render.hxx"
#include "imgui_submit.hxx"
#include "loading_dynamic.hxx"
#include "prepare_uniforms.hxx"
#include "present.hxx"
#include "readback.hxx"
#include "reset_pools.hxx"
#include "setup_gpu_signal.hxx"
#include "update.hxx"
#include "schedule_all.hxx"

#define TRACY_ARTIFICIAL_DELAY 1ms

namespace engine::frame {

void _begin(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<task::Task> rendering_yarn_end,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> data,
  Use<engine::session::Data::GLFW> glfw,
  Use<engine::display::Data::PresentationFailureState> presentation_failure_state, // @Cleanup use Ref
  Use<engine::display::Data::SwapchainDescription> swapchain_description, // @Cleanup use ref
  Own<engine::display::Data::FrameInfo> latest_frame
) {
  ZoneScopedC(0xFF0000);
  FrameMark;
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
  latest_frame->is_sequential = (latest_frame->number > 0);
  latest_frame->timeline_semaphore_value = latest_frame->number + 1;
  latest_frame->inflight_index = (
    latest_frame->number % swapchain_description->image_count
  );

  // @Cleanup: should put FrameInfo inside FrameData.
  auto frame_info = new engine::display::Data::FrameInfo(*latest_frame);
  auto frame_data = new engine::misc::FrameData {};

  #ifdef ENGINE_DEVELOPER
    {
      auto fc = &session->frame_control;
      auto lock = std::scoped_lock(fc->mutex);
      frame_info->directives.should_capture_screenshot = fc->directives.should_capture_screenshot;
      frame_info->directives.screenshot_path = fc->directives.screenshot_path;
      fc->directives = {};
    }
  #endif

  auto task_setup_gpu_signal = defer(
    task::create(
      setup_gpu_signal,
      session.ptr,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &data->frame_finished_semaphore,
      frame_info
    )
  );
  auto frame_tasks = new std::vector<task::Task *>({
    task::create(
      handle_window_events,
      &session->glfw,
      &session->state,
      &frame_data->update_data
    ),
    task::create(
      update,
      &frame_data->update_data,
      frame_info,
      &data->readback,
      &session->state
    ),
    task::create(
      reset_pools,
      &session->vulkan.core,
      &data->command_pools,
      &data->descriptor_pools,
      frame_info
    ),
    task::create(
      acquire,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info
    ),
    task::create(
      prepare_uniforms,
      &session->vulkan.core,
      &data->swapchain_description,
      frame_info,
      &session->state,
      &data->common,
      &data->gpass,
      &data->lpass
    ),
    task::create(
      generate_render_list,
      session.ptr,
      &session->scene,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      &frame_data->render_list
    ),
    task::create(
      graphics_render,
      session.ptr,
      &session->state,
      &session->vulkan.core,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->descriptor_pools,
      &data->prepass,
      &data->gpass,
      &data->lpass,
      &data->pass_secondary_geometry,
      &data->pass_indirect_light_secondary,
      &data->pass_directional_light_secondary,
      &data->pass_probe_light_update,
      &data->pass_probe_depth_update,
      &data->pass_indirect_light,
      &data->finalpass,
      &data->zbuffer,
      &data->gbuffer,
      &data->lbuffer,
      &data->zbuffer2,
      &data->gbuffer2,
      &data->lbuffer2,
      &data->probe_light_map,
      &data->probe_depth_map,
      &data->final_image,
      &session->vulkan.prepass,
      &session->vulkan.gpass,
      &session->vulkan.lpass,
      &session->vulkan.pass_secondary_geometry,
      &session->vulkan.pass_indirect_light_secondary,
      &session->vulkan.pass_directional_light_secondary,
      &session->vulkan.pass_probe_light_update,
      &session->vulkan.pass_probe_depth_update,
      &session->vulkan.pass_indirect_light,
      &session->vulkan.finalpass,
      &session->vulkan.fullscreen_quad,
      &frame_data->render_list,
      &frame_data->graphics_data
    ),
    task::create(
      graphics_submit,
      &session->vulkan.queue_work,
      &data->graphics_finished_semaphore,
      frame_info,
      &frame_data->graphics_data
    ),
    task::create(
      imgui_new_frame,
      &session->imgui_context,
      &data->imgui_backend,
      &session->glfw
    ),
    task::create(
      imgui_populate,
      session.ptr,
      data.ptr,
      &session->imgui_context,
      &frame_data->imgui_reactions,
      &session->meta_meshes,
      &session->meta_textures,
      &session->state
    ),
    task::create(
      imgui_render,
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
      imgui_submit,
      &session->vulkan.queue_work,
      &data->graphics_finished_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      &frame_data->imgui_data
    ),
    task::create(
      compose_render,
      session.ptr,
      data.ptr,
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
      compose_submit,
      &session->vulkan.queue_work,
      &data->presentation,
      &data->imgui_finished_semaphore,
      frame_info,
      &frame_data->compose_data
    ),
    task::create(
      present,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info,
      &session->vulkan.queue_present
    ),
    task::create(
      readback,
      &session->vulkan.core,
      &session->vulkan.queue_work,
      &data->presentation,
      &data->frame_finished_semaphore,
      &data->lbuffer,
      &data->readback,
      &data->command_pools,
      &data->swapchain_description,
      frame_info
    ),
    task::create(
      loading_dynamic,
      session.ptr,
      &session->guid_counter,
      &session->meta_meshes,
      &session->meta_textures,
      &frame_data->imgui_reactions
    ),
    task::create(
      cleanup,
      frame_info,
      frame_data
    ),
    task::create(
      schedule_all,
      rendering_yarn_end.ptr,
      session.ptr,
      data.ptr
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

void schedule_all(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<task::Task> rendering_yarn_end,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display
) {
  ZoneScoped;

  #ifdef ENGINE_DEVELOPER
    #ifdef TRACY_ARTIFICIAL_DELAY
      if (TracyIsConnected) {
        ZoneScopedN("artificial_delay");
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(TRACY_ARTIFICIAL_DELAY);
      }
    #endif
  #endif

  auto task_frame = lib::task::create(
    _begin,
    rendering_yarn_end.ptr,
    session.ptr,
    display.ptr,
    &session->glfw,
    &display->presentation_failure_state,
    &display->swapchain_description,
    &display->latest_frame
  );

  #ifdef ENGINE_DEVELOPER
    lib::Task *signal = nullptr;
    if (session->frame_control.enabled) {
      auto fc = &session->frame_control;
      auto lock = std::scoped_lock(fc->mutex);
      if (fc->allowed_count > 0) {
        fc->allowed_count--;
      } else {
        signal = fc->signal_allowed = lib::task::create_yarn_signal();

        if (fc->signal_done) {
          lib::task::signal(ctx->runner, fc->signal_done);
          fc->signal_done = nullptr;
        }
      }

      auto deferred_frame = defer(task_frame);

      // inject still under the FC mutex
      lib::task::inject(ctx->runner, {
        deferred_frame.first,
      }, {
        .new_dependencies = {
          { signal, deferred_frame.first },
        },
      });
    }
  #else
    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_frame,
    });
  #endif
}

} // namespace
