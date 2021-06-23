#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <TracyVulkan.hpp>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/lib/debug_camera.hxx>

struct SessionData : lib::task::ParentResource {
  struct GLFW {
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  struct Vulkan : lib::task::ParentResource {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR window_surface;
    VkPhysicalDevice physical_device;

    struct Core {
      VkDevice device;
      const VkAllocationCallbacks *allocator;
      tracy::VkCtx *tracy_context;
    } core;

    struct CoreProperties {
      VkPhysicalDeviceProperties basic;
      VkPhysicalDeviceMemoryProperties memory;
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing;
    } properties;

    VkQueue queue_present;
    VkQueue queue_work; // graphics, compute, transfer
    uint32_t queue_family_index;

    VkCommandPool tracy_setup_command_pool;
    VkDescriptorPool common_descriptor_pool;

    lib::gfx::multi_alloc::Instance multi_alloc;

    struct Example {
      struct Geometry {
        lib::gfx::multi_alloc::StakeBuffer vertex_stake;
        size_t triangle_count;
      } geometry;
      struct FullscreenQuad {
        lib::gfx::multi_alloc::StakeBuffer vertex_stake;
        size_t triangle_count;
      } fullscreen_quad;
      struct GPass {
        VkDescriptorSetLayout descriptor_set_layout;
        VkDescriptorSet descriptor_set;
        VkPipelineLayout pipeline_layout;
      } gpass;
      struct LPass {
        VkPipelineLayout pipeline_layout;
      } lpass;
    } example;
  } vulkan;

  struct ImguiContext {
    // ImGui::* and ImGui_ImplGlfw_*
    uint8_t padding;
  } imgui_context;

  lib::gpu_signal::Support gpu_signal_support;

  struct Info {
    size_t worker_count;
  } info;

  struct State {
    bool show_imgui;
    bool is_fullscreen;
    lib::debug_camera::State debug_camera;
  } state;
};
