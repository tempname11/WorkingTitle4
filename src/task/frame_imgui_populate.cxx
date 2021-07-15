#include <imgui.h>
#include "frame_imgui_populate.hxx"

TASK_DECL {
  if (state->show_imgui) {
    ImGui::ShowDemoWindow(nullptr);
  }
}
