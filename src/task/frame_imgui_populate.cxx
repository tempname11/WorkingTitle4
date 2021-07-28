#include <imgui.h>
#include "frame_imgui_populate.hxx"

TASK_DECL {
  if (state->show_imgui) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Demo", "CTRL-D", &state->show_imgui_window_demo);
      ImGui::MenuItem("Groups", "CTRL-G", &state->show_imgui_window_groups);
      ImGui::MenuItem("Meshes", "CTRL-M", &state->show_imgui_window_meshes);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    if (state->show_imgui_window_demo) {
      ImGui::ShowDemoWindow(&state->show_imgui_window_demo);
    }

    if (state->show_imgui_window_groups) {
      ImGui::Begin("Groups", &state->show_imgui_window_groups);
      ImGui::BeginTable("table_groups", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Status");
      ImGui::TableHeadersRow();
      for (auto &item : groups->items) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          item.name.c_str(),
          item.name.c_str() + item.name.size()
        );
        ImGui::TableNextColumn();
        if (item.status == SessionData::Groups::Status::Loading) {
          ImGui::Text("[loading]");
        }
        if (item.status == SessionData::Groups::Status::Ready) {
          ImGui::Text("[ready]");
        }
      }
      ImGui::EndTable();
      ImGui::End();
    }

    if (state->show_imgui_window_meshes) {
      ImGui::Begin("Meshes", &state->show_imgui_window_meshes);
      ImGui::BeginTable("table_meshes", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Status");
      ImGui::TableSetupColumn("Actions");
      ImGui::TableHeadersRow();
      for (auto &item : meta_meshes->items) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          item.second.path.c_str(),
          item.second.path.c_str() + item.second.path.size()
        );
        ImGui::TableNextColumn();
        if (item.second.status == SessionData::MetaMeshes::Status::Loading) {
          ImGui::Text("[loading]");
        }
        if (item.second.status == SessionData::MetaMeshes::Status::Ready) {
          ImGui::Text("[ready]");
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("Reload")) {
          imgui_reactions->reload_mesh_id = item.first;
        }
      }
      ImGui::EndTable();
      ImGui::End();
    }
  }
}
