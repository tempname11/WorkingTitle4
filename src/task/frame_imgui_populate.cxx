#include <cinttypes>
#include <imgui.h>
#include <nfd.h>
#ifdef WINDOWS
  #define NOMINMAX
  #include <windows.h>
  #undef NOMINMAX
#endif
#include <misc/cpp/imgui_stdlib.h>
#include <src/engine/loading/group.hxx>
#include "frame_imgui_populate.hxx"

engine::loading::group::GroupDescription default_group = {
  .name = "New group",
};

namespace ImGuiX {
  enum class DialogType {
    Open,
    Save,
  };

  void InputPath(std::string *path, char const *label, DialogType type = DialogType::Open) {
    ImGui::PushID(label);
    ImGui::InputText("", path);
    ImGui::SameLine();
    if (ImGui::Button("...")) {
      nfdchar_t *npath = nullptr;
      auto result = (type == DialogType::Save
        ? NFD_SaveDialog(nullptr, nullptr, &npath)
        : NFD_OpenDialog(nullptr, nullptr, &npath)
      );
      if (result == NFD_OKAY) {
        // @Note: this is a bit hairy, for relative path extraction
        #ifdef WINDOWS
          char cwd[1024];
          auto result = GetCurrentDirectory(1024, cwd);
          assert(result != 0);
          assert(result < 1024);
        #endif
        assert(cwd != nullptr);
        char const *ptr_npath = npath;
        char const *ptr_cwd = cwd;
        while (true
          && *ptr_npath == *ptr_cwd
          && *ptr_npath != 0
          && *ptr_cwd != 0
        ) {
          ptr_npath++;
          ptr_cwd++;
        }
        bool can_use_relative_path = (true
          && *ptr_cwd == 0
          && (false
            || *ptr_npath == '/'
            || *ptr_npath == '\\'
          )
        );
        if (can_use_relative_path) {
          ptr_npath++;
        } else {
          ptr_npath = npath;
        }

        path->resize(strlen(ptr_npath));
        strcpy(path->data(), ptr_npath);
        free(npath);
      } else {
        assert(result == NFD_CANCEL);
      }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::PopID();
  }
}

engine::loading::group::ItemDescription default_group_item = {
  /*
  .path_mesh = "assets/mesh.t05",
  .path_albedo = "assets/texture/albedo.jpg",
  .path_normal = "assets/texture/normal.jpg",
  .path_romeao = "assets/texture/romeao.png",
  */
  .path_mesh = "assets/r0.t05",
  .path_albedo = "assets/texture-1px/albedo.png",
  .path_normal = "assets/texture-1px/normal.png",
  .path_romeao = "assets/texture-1px/romeao.png",
};

TASK_DECL {
  if (state->show_imgui) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Demo", "CTRL-D", &state->show_imgui_window_demo);
      ImGui::MenuItem("Groups", "CTRL-G", &state->show_imgui_window_groups);
      ImGui::MenuItem("Meshes", "CTRL-M", &state->show_imgui_window_meshes);
      ImGui::MenuItem("GPU Memory", "CTRL-Q", &state->show_imgui_window_gpu_memory);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    if (state->show_imgui_window_demo) {
      ImGui::ShowDemoWindow(&state->show_imgui_window_demo);
    }

    if (state->show_imgui_window_groups) {
      std::shared_lock lock(session->groups.rw_mutex);
      ImGui::Begin("Groups", &state->show_imgui_window_groups);
      ImGui::BeginTable("table_groups", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Actions");
      ImGui::TableHeadersRow();
      for (auto &pair : session->groups.items) {
        auto group_id = pair.first;
        auto item = &pair.second;
        if (item->lifetime.ref_count == 0) {
          continue;
        }
        ImGui::PushID(group_id);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          item->name.c_str(),
          item->name.c_str() + item->name.size()
        );
        ImGui::TableNextColumn();
        if (ImGui::Button("...")) {
          ImGui::OpenPopup("Group actions");
        }
        bool clicked_add_item = false;
        bool clicked_save = false;
        if (ImGui::BeginPopup("Group actions")) {
          if (ImGui::Selectable("Remove")) {
            imgui_reactions->removed_group_id = group_id;
          }
          if (ImGui::Selectable("Add item...")) {
            clicked_add_item = true;
          }
          if (ImGui::Selectable("Save...")) {
            clicked_save = true;
          }
          ImGui::EndPopup();
        }
        if (clicked_add_item) {
          ImGui::OpenPopup("Add an item to the group");
        }
        if (clicked_save) {
          ImGui::OpenPopup("Save group");
        }
        if (ImGui::BeginPopupModal("Add an item to the group", NULL, 0)) {
          static engine::loading::group::ItemDescription desc = {};
          ImGuiX::InputPath(&desc.path_mesh, "Path to mesh");
          ImGuiX::InputPath(&desc.path_albedo, "Path to `albedo` texture");
          ImGuiX::InputPath(&desc.path_normal, "Path to `normal` texture");
          ImGuiX::InputPath(&desc.path_romeao, "Path to `romeao` texture");
          if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            imgui_reactions->added_item_to_group_id = group_id;
            imgui_reactions->added_item_to_group_description = new engine::loading::group::ItemDescription(desc);
            desc = {};
          }
          ImGui::SameLine();
          if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            desc = {};
          }
          ImGui::SameLine();
          if (ImGui::Button("Reset to default")) {
            desc = default_group_item;
          }
          ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Save group", NULL, 0)) {
          static std::string path;
          ImGuiX::InputPath(&path, "Path", ImGuiX::DialogType::Save);
          if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            engine::loading::group::save(
              ctx,
              &path,
              group_id,
              session
            );
            path = {};
          }
          ImGui::SameLine();
          if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            path = {};
          }
          ImGui::EndPopup();
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
      if (ImGui::Button("New...")) {
        ImGui::OpenPopup("Create a new group");
      }
      ImGui::SameLine();
      if (ImGui::Button("Load...")) {
        ImGui::OpenPopup("Load group");
      }
      if (ImGui::BeginPopupModal("Create a new group", NULL, 0)) {
        static engine::loading::group::GroupDescription desc = {};
        ImGui::InputText("Group name", &desc.name);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          imgui_reactions->created_group_description = new engine::loading::group::GroupDescription(desc);
          desc = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          desc = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to default")) {
          desc = default_group;
        }

        ImGui::EndPopup();
      }
      if (ImGui::BeginPopupModal("Load group", NULL, 0)) {
        static std::string path;
        ImGuiX::InputPath(&path, "Path");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          engine::loading::group::load(
            ctx,
            &path,
            session
          );
          path = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          path = {};
        }
        ImGui::EndPopup();
      }
      ImGui::End();
    }

    if (state->show_imgui_window_meshes) {
      ImGui::Begin("Meshes", &state->show_imgui_window_meshes);
      ImGui::BeginTable("table_meshes", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Status");
      ImGui::TableSetupColumn("Refcount");
      ImGui::TableSetupColumn("Actions");
      ImGui::TableHeadersRow();
      for (auto &pair : meta_meshes->items) {
        auto item = &pair.second;
        auto mesh_id = pair.first;
        ImGui::PushID(mesh_id);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          item->path.c_str(),
          item->path.c_str() + item->path.size()
        );
        ImGui::TableNextColumn();
        if (item->status == SessionData::MetaMeshes::Status::Loading) {
          ImGui::Text("[loading]");
        }
        if (item->status == SessionData::MetaMeshes::Status::Ready) {
          if (item->invalid) {
            ImGui::Text("<invalid>");
          } else {
            ImGui::Text("[ready]");
          }
        }
        ImGui::TableNextColumn();
        ImGui::Text("%zu", item->ref_count);
        ImGui::TableNextColumn();

        { // reload button
          auto disabled = item->will_have_reloaded != nullptr;
          if (disabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          }
          if (ImGui::Button("Reload") && !disabled) {
            imgui_reactions->reloaded_mesh_id = mesh_id;
          }
          if (disabled) {
            ImGui::PopStyleVar();
          }
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
      ImGui::End();
    }

    if (state->show_imgui_window_gpu_memory) {
      ImGui::Begin("GPU memory", &state->show_imgui_window_gpu_memory);
      if (ImGui::TreeNode("GPU_LOCAL")) {
        auto it = &session->vulkan.allocator_gpu_local;
        std::shared_lock lock(it->rw_mutex);

        if (
          ImGui::TreeNode(
            &it->dedicated_allocations,
            "Dedicated: %zu allocations",
            it->dedicated_allocations.size()
          )
         ) {
          for (auto &item : it->dedicated_allocations) {
            ImGui::Text("ID = %" PRIi64 ", size = %zu", item.id, item.size);
          }
          ImGui::TreePop();
        }

        if (
          ImGui::TreeNode(
            &it->regions,
            "Shared: %zu regions",
            it->regions.size()
          )
         ) {
          for (auto &item : it->regions) {
            if (
              ImGui::TreeNode(
                &item,
                "%zu active suballocations, occupied = %zu / %zu",
                item.total_active_suballocations,
                item.size_occupied,
                it->size_region
              )
            ) {
              for (auto &item : item.suballocations) {
                ImGui::Text("ID = %" PRIi64 ", offset = %zu, size = %zu", item.id, item.offset, item.size);
              }
              ImGui::TreePop();
            }
          }
          ImGui::TreePop();
        }

        ImGui::TreePop();
      }
      ImGui::End();
    }
  }
}
