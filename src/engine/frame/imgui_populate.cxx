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
#include <src/engine/tools/cube_writer.hxx>
#include <src/engine/tools/gltf_converter.hxx>
#include <src/engine/tools/voxel_converter.hxx>
#include "imgui_populate.hxx"

namespace engine::frame {

engine::loading::group::GroupDescription default_group = {
  .name = "New group",
};

namespace ImGuiX {
  void Checkbox32(char const* label, uint32_t *v) {
    bool f = *v;
    ImGui::Checkbox(label, &f);
    *v = f;
  }
  enum class DialogType {
    OpenFile,
    SaveFile,
    PickFolder
  };

  void InputPath(std::string *path, char const *label, DialogType type = DialogType::OpenFile) {
    ImGui::PushID(label);
    ImGui::InputText("", path);
    ImGui::SameLine();
    if (ImGui::Button("...")) {
      nfdchar_t *npath = nullptr;
      auto result = (
        type == DialogType::SaveFile ? NFD_SaveDialog(nullptr, nullptr, &npath) :
        type == DialogType::OpenFile ? NFD_OpenDialog(nullptr, nullptr, &npath) :
        type == DialogType::PickFolder ? NFD_PickFolder(nullptr, &npath) :
        NFD_ERROR
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
  .path_mesh = "assets/mesh.t06",
  .path_albedo = "assets/texture-1px/albedo.png",
  .path_normal = "assets/texture-1px/normal.png",
  .path_romeao = "assets/texture-1px/romeao.png",
};

void imgui_allocator(
  Ref<lib::gfx::Allocator> it
) {
  ZoneScoped;
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
}

void imgui_populate(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display,
  Own<engine::session::Data::ImguiContext> imgui,
  Own<engine::misc::ImguiReactions> imgui_reactions,
  Use<engine::session::Data::MetaMeshes> meta_meshes,
  Use<engine::session::Data::MetaTextures> meta_textures,
  Own<engine::session::Data::State> state
) {
  if (state->show_imgui) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Demo", "CTRL-D", &state->show_imgui_window_demo);
      ImGui::MenuItem("Groups", "CTRL-G", &state->show_imgui_window_groups);
      ImGui::MenuItem("Meshes", "CTRL-M", &state->show_imgui_window_meshes);
      ImGui::MenuItem("Textures", "CTRL-T", &state->show_imgui_window_textures);
      ImGui::MenuItem("GPU Memory", "CTRL-Q", &state->show_imgui_window_gpu_memory);
      ImGui::MenuItem("Tools", "CTRL-W", &state->show_imgui_window_tools);
      ImGui::MenuItem("Flags", "CTRL-F", &state->show_imgui_window_flags);
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
          ImGuiX::InputPath(&path, "Path", ImGuiX::DialogType::SaveFile);
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
          auto task = engine::loading::group::load(
            ctx,
            &path,
            session
          );
          ctx->new_tasks.insert(ctx->new_tasks.end(), {
            task,
          });
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
      ImGui::BeginTable("table_meshes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Path");
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
        if (item->status == engine::session::Data::MetaMeshes::Status::Loading) {
          ImGui::Text("[loading]");
        }
        if (item->status == engine::session::Data::MetaMeshes::Status::Ready) {
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

    if (state->show_imgui_window_textures) {
      ImGui::Begin("Textures", &state->show_imgui_window_textures);
      ImGui::BeginTable("table_textures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
      ImGui::TableSetupColumn("Path");
      ImGui::TableSetupColumn("Status");
      ImGui::TableSetupColumn("Refcount");
      ImGui::TableSetupColumn("Actions");
      ImGui::TableHeadersRow();
      for (auto &pair : meta_textures->items) {
        auto item = &pair.second;
        auto texture_id = pair.first;
        ImGui::PushID(texture_id);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          item->path.c_str(),
          item->path.c_str() + item->path.size()
        );

        ImGui::TableNextColumn();
        if (item->status == engine::session::Data::MetaTextures::Status::Loading) {
          ImGui::Text("[loading]");
        }
        if (item->status == engine::session::Data::MetaTextures::Status::Ready) {
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
            imgui_reactions->reloaded_texture_id = texture_id;
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
      if (ImGui::TreeNode("UPLOADER.DEVICE")) {
        imgui_allocator(&session->vulkan.uploader.allocator_device);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("UPLOADER.HOST")) {
        imgui_allocator(&session->vulkan.uploader.allocator_host);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("BLAS.MAIN")) {
        imgui_allocator(&session->vulkan.blas_storage.allocator_blas);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("BLAS.SCRATCH")) {
        imgui_allocator(&session->vulkan.blas_storage.allocator_scratch);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("DISPLAY.DEDICATED")) {
        imgui_allocator(&display->allocator_dedicated);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("DISPLAY.SHARED")) {
        imgui_allocator(&display->allocator_shared);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("DISPLAY.HOST")) {
        imgui_allocator(&display->allocator_host);
        ImGui::TreePop();
      }
      ImGui::End();
    }

    if (state->show_imgui_window_tools) {
      ImGui::Begin("Tools", &state->show_imgui_window_tools);
      if (ImGui::Button("GLTF Converter")) {
        ImGui::OpenPopup("GLTF Converter");
      }
      if (ImGui::Button("VOX Converter")) {
        ImGui::OpenPopup("VOX Converter");
      }
      if (ImGui::Button("Cube Writer")) {
        ImGui::OpenPopup("Cube Writer");
      }
      if (ImGui::BeginPopupModal("GLTF Converter", NULL, 0)) {
        static std::string path_gltf;
        static std::string path_folder;
        ImGuiX::InputPath(&path_gltf, "GLTF Path");
        ImGuiX::InputPath(&path_folder, "Folder Path", ImGuiX::DialogType::PickFolder);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          tools::gltf_converter(path_gltf.c_str(), path_folder.c_str());
          ImGui::CloseCurrentPopup();
          path_gltf = {};
          path_folder = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          path_gltf = {};
          path_folder = {};
        }
        ImGui::EndPopup();
      }
      if (ImGui::BeginPopupModal("VOX Converter", NULL, 0)) {
        static std::string path_vox;
        static std::string path_folder;
        static bool enable_marching_cubes;
        static bool enable_random_voxels;
        ImGuiX::InputPath(&path_vox, "VOX Path");
        ImGuiX::InputPath(&path_folder, "Folder Path", ImGuiX::DialogType::PickFolder);
        ImGui::Checkbox("enable marching cubes", &enable_marching_cubes);
        ImGui::Checkbox("enable random voxels", &enable_random_voxels);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          tools::voxel_converter(
            path_vox.c_str(),
            path_folder.c_str(),
            enable_marching_cubes,
            enable_random_voxels
          );
          ImGui::CloseCurrentPopup();
          path_vox = {};
          path_folder = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          path_vox = {};
          path_folder = {};
        }
        ImGui::EndPopup();
      }
      if (ImGui::BeginPopupModal("Cube Writer", NULL, 0)) {
        static std::string path_t06;
        ImGuiX::InputPath(&path_t06, "t06 Path", ImGuiX::DialogType::SaveFile);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          tools::cube_writer(path_t06.c_str());
          ImGui::CloseCurrentPopup();
          path_t06 = {};
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          path_t06 = {};
        }
        ImGui::EndPopup();
      }
      ImGui::End();
    }

    if (state->show_imgui_window_flags) {
      auto it = &state->ubo_flags;
      ImGui::Begin("Flags", &state->show_imgui_window_flags);
      ImGuiX::Checkbox32("Disable direct lighting", &it->disable_direct_lighting);
      ImGuiX::Checkbox32("Disable direct shadows", &it->disable_direct_shadows);
      ImGuiX::Checkbox32("Disable indirect lighting", &it->disable_indirect_lighting);
      ImGuiX::Checkbox32("Disable indirect bounces", &it->disable_indirect_bounces);
      ImGuiX::Checkbox32("Disable indirect skips", &it->disable_indirect_skips);
      ImGuiX::Checkbox32("Disable eye adaptation", &it->disable_eye_adaptation);
      ImGui::SameLine();
      ImGui::SliderFloat(
        "Avg.",
        &state->luminance_moving_average,
        0.001f,
        1000.0f,
        "%.3f",
        ImGuiSliderFlags_Logarithmic
      );
      ImGuiX::Checkbox32("Disable motion blur", &it->disable_motion_blur);
      ImGuiX::Checkbox32("Disable TAA", &it->disable_TAA);
      ImGui::SameLine();
      ImGui::SliderFloat(
        "Dist.",
        &state->taa_distance,
        0.0f,
        2.0f,
        "%.3f"
      );
      ImGuiX::Checkbox32("Disable sky", &it->disable_sky);
      ImGuiX::Checkbox32("Debug A", &it->debug_A);
      ImGuiX::Checkbox32("Debug B", &it->debug_B);
      ImGuiX::Checkbox32("Debug C", &it->debug_C);
      ImGui::VSliderFloat(
        "Y",
        ImVec2(20.0f, 128.0f),
        &state->sun_position_xy.y,
        -1.0f,
        1.0f,
        ""
      );
      ImGui::SameLine();
      ImGui::BeginChild("sun_xy", ImVec2(120.0f, 100.0f));
      ImGui::SetCursorPosX(30.0f);
      ImGui::SetCursorPosY(50.0f);
      ImGui::Text("X: %.3f", state->sun_position_xy.x);
      ImGui::SetCursorPosX(30.0f);
      ImGui::Text("Y: %.3f", state->sun_position_xy.y);
      ImGui::EndChild();
      ImGui::SetNextItemWidth(128.0f + 24.0f);
      ImGui::Indent(24.0f);
      ImGui::SliderFloat(
        "X",
        &state->sun_position_xy.x,
        -1.0f,
        1.0f,
        ""
      );
      ImGui::SliderFloat(
        "Movement speed",
        &state->movement_speed,
        0.25f,
        256.0f,
        "%.2f",
        ImGuiSliderFlags_Logarithmic
      );
      ImGui::SliderFloat(
        "Sun irradiance",
        &state->sun_irradiance,
        0.005f,
        5000.0f,
        "%.3f",
        ImGuiSliderFlags_Logarithmic
      );
      ImGui::End();
    }
  }
}

} // namespace
