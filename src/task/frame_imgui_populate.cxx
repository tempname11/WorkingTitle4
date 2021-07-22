#include <imgui.h>
#include "frame_imgui_populate.hxx"

TASK_DECL {
  if (state->show_imgui) {
    ImGui::ShowDemoWindow(nullptr);

    ImGui::Begin("Groups");
    for (auto &item : groups->items) {
      ImGui::TextUnformatted(
        item.name.c_str(),
        item.name.c_str() + item.name.size()
      );
      ImGui::SameLine();
      if (ImGui::Button("Reload")) {
        imgui_reactions->reload_group_id = item.group_id;
      }
    }
    ImGui::End();
  }
}
