#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "imgui_new_frame.hxx"

namespace engine::frame {

// This is done by the main thread because of GLFW usage.

void imgui_new_frame(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data::ImguiContext> imgui,
  Own<engine::display::Data::ImguiBackend> imgui_backend,
  Own<engine::session::Data::GLFW> glfw /* hacky way to prevent KeyMods bug. */
) {
  ZoneScoped;

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

} // namespace
