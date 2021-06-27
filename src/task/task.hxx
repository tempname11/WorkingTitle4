#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include <src/lib/task.hxx>
#include <src/global.hxx>

namespace task = lib::task;
namespace usage = lib::usage;

void defer(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<task::Task> task
);

void defer_many(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<std::vector<task::Task *>> tasks
);

void rendering_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Some<SessionData> session,
  usage::Full<RenderingData> data
);

void frame_handle_window_events(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData::GLFW> glfw,
  usage::Full<SessionData::State> session_state,
  usage::Full<UpdateData> update
);

void frame_update(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<UpdateData> update,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<SessionData::State> session_state
);

void frame_setup_gpu_signal(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<lib::gpu_signal::Support> gpu_signal_support,
  usage::Full<VkSemaphore> frame_rendered_semaphore,
  usage::Full<RenderingData::InflightGPU> inflight_gpu,
  usage::Some<RenderingData::FrameInfo> frame_info
);

void frame_reset_pools(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info
);

void frame_graphics_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Some<VkSemaphore> graphics_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<GraphicsData> data
);

void frame_graphics_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<RenderingData::Prepass> prepass,
  usage::Some<RenderingData::GPass> gpass,
  usage::Some<RenderingData::LPass> lpass,
  usage::Some<RenderingData::Finalpass> finalpass,
  usage::Some<RenderingData::ZBuffer> zbuffer,
  usage::Some<RenderingData::GBuffer> gbuffer,
  usage::Some<RenderingData::LBuffer> lbuffer,
  usage::Some<RenderingData::FinalImage> final_image,
  usage::Some<SessionData::Vulkan::GPass> s_gpass,
  usage::Some<SessionData::Vulkan::LPass> s_lpass,
  usage::Some<SessionData::Vulkan::Finalpass> s_finalpass,
  usage::Some<SessionData::Vulkan::Geometry> geometry,
  usage::Some<SessionData::Vulkan::FullscreenQuad> fullscreen_quad,
  usage::Full<GraphicsData> data
);

void frame_prepare_uniforms(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<SessionData::State> session_state,
  usage::Full<RenderingData::GPass> gpass,
  usage::Full<RenderingData::LPass> lpass
);

void frame_imgui_populate(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Some<SessionData::State> state
);

void frame_imgui_new_frame(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
);

void frame_imgui_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::ImguiContext> imgui_context,
  usage::Full<RenderingData::ImguiBackend> imgui_backend,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
);

void frame_imgui_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Some<VkSemaphore> graphics_finished_semaphore,
  usage::Some<VkSemaphore> imgui_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
);

void frame_acquire(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::FrameInfo> frame_info
);

void frame_compose_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<RenderingData::FinalImage> final_image,
  usage::Full<ComposeData> data
);

void frame_compose_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<VkSemaphore> frame_rendered_semaphore,
  usage::Some<VkSemaphore> imgui_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ComposeData> data
);

void frame_present(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<VkQueue> queue_present
);

void frame_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<RenderingData::FrameInfo> frame_info
);

void rendering_has_finished(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> rendering_yarn_end
);

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
);

void rendering_imgui_setup_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
);

void session_iteration_try_rendering(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Full<SessionData> session
);

void session_iteration(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<task::Task> session_yarn_end,
  usage::Some<SessionData> data
);

void session_cleanup(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData> session
);

void session(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::None<size_t> worker_count
);