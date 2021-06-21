#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "task.hxx"

void rendering_frame_imgui_new_frame(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
) {
  // main thread because of glfw usage
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}
