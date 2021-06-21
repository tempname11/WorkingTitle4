#include <imgui.h>
#include "task.hxx"

void rendering_frame_imgui_populate(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Some<SessionData::State> state
) {
  if (state->show_imgui) {
    ImGui::ShowDemoWindow(nullptr);
  }
}
