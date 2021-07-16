#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/debug_camera.hxx>
#include "session.hxx"

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
  };

  std::vector<Item> items;
};

struct ImguiReactions {
  bool reload;
};
