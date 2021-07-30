#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/guid.hxx>
#include <src/lib/task.hxx>
#include <src/lib/debug_camera.hxx>
#include "loading/group.hxx"

namespace engine::misc {

struct ComposeData {
  VkCommandBuffer cmd;
};

struct GraphicsData {
  VkCommandBuffer cmd;
};

struct ImguiData {
  VkCommandBuffer cmd;
};

struct GlfwUserData {
  SessionData::State *state;
};

struct UpdateData {
  lib::debug_camera::Input debug_camera_input;
};

struct RenderList {
  struct Item {
    glm::mat4 transform;
    VkBuffer mesh_buffer;
    uint32_t mesh_vertex_count;
    VkImageView texture_albedo_view;
    VkImageView texture_normal_view;
    VkImageView texture_romeao_view;
  };

  std::vector<Item> items;
};

struct ImguiReactions {
  lib::GUID reload_mesh_id;
  lib::GUID reload_texture_id;
  engine::loading::group::SimpleItemDescription *load_group_description;
};

struct FrameData : lib::task::ParentResource {
  ComposeData compose_data;
  GraphicsData graphics_data;
  ImguiData imgui_data;
  UpdateData update_data;
  RenderList render_list;
  ImguiReactions imgui_reactions;
};

} // namespace
