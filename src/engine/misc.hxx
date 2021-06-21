#pragma once
#include <vulkan/vulkan.h>
#include <src/lib/debug_camera.hxx>
#include "session.hxx"

struct ComposeData {
  VkCommandBuffer cmd;
};

struct ExampleData {
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
