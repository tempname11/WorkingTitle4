#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/guid.hxx>
#include <src/lib/task.hxx>
#include <src/lib/debug_camera.hxx>
#include "system/grup/group.hxx"

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
  engine::session::Data::State *state;
};

struct UpdateData {
  lib::debug_camera::Input debug_camera_input;
};

struct RenderList {
  struct Item {
    glm::mat4 transform;
    VkBuffer mesh_buffer;
    uint32_t mesh_index_count;
    uint32_t mesh_buffer_offset_indices;
    uint32_t mesh_buffer_offset_vertices;
    VkDeviceAddress mesh_buffer_address;
    VkDeviceAddress blas_address;
    VkImageView texture_albedo_view;
    VkImageView texture_normal_view;
    VkImageView texture_romeao_view;
  };

  std::vector<Item> items;
};

struct ImguiReactions {
  lib::GUID reloaded_mesh_id;
  lib::GUID reloaded_texture_id;
  engine::system::grup::group::GroupDescription *created_group_description;
  lib::GUID added_item_to_group_id;
  engine::system::grup::group::ItemDescription *added_item_to_group_description;
  lib::GUID removed_group_id;
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
