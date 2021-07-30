#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <src/engine/loading/group.hxx>
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
      ImGui::BeginTable("table_groups", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Status");
      ImGui::TableSetupColumn("Actions");
      ImGui::TableHeadersRow();
      for (auto &pair : groups->items) {
        auto group_id = pair.first;
        auto &item = pair.second;
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
        ImGui::TableNextColumn();
        if (ImGui::Button("Remove")) {
          imgui_reactions->removed_group_id = group_id;
        }
      }
      ImGui::EndTable();
      if (ImGui::Button("New...")) {
        ImGui::OpenPopup("popup_new");
      }
      if (ImGui::BeginPopupModal("popup_new", NULL, 0)) {
        static engine::loading::group::SimpleItemDescription desc = {
          .name = "Example Static Group",
          .path_mesh = "assets/mesh.t05",
          .path_albedo = "assets/texture/albedo.jpg",
          .path_normal = "assets/texture/normal.jpg",
          .path_romeao = "assets/texture/romeao.png",
        };
        ImGui::Text("Hello, world!");
        ImGui::InputText("Group name", &desc.name);
        ImGui::InputText("Path to mesh", &desc.path_mesh);
        ImGui::InputText("Path to `albedo` texture", &desc.path_albedo);
        ImGui::InputText("Path to `normal` texture", &desc.path_normal);
        ImGui::InputText("Path to `romeao` texture", &desc.path_romeao);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          imgui_reactions->load_group_description = new engine::loading::group::SimpleItemDescription(desc);
          desc = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          desc = {};
        }

        ImGui::EndPopup();
      }
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
