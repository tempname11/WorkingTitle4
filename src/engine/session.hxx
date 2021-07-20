#pragma once
#include <mutex>
#include <unordered_set>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <TracyVulkan.hpp>
#include <src/engine/common/texture.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/lib/task.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/lib/debug_camera.hxx>

struct SessionData : lib::task::ParentResource {
  struct GLFW {
    bool ready;
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  struct UnfinishedYarns {
    std::mutex mutex;
    std::unordered_set<lib::Task *> set;
  } unfinished_yarns;

  struct Scene {
    struct Item {
      glm::mat4 transform;
      size_t mesh_index;
      size_t texture_albedo_index;
      size_t texture_normal_index;
      size_t texture_romeao_index;
    };

    std::vector<Item> items;
  } scene;

  struct Vulkan : lib::task::ParentResource {
    bool ready;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR window_surface;
    VkPhysicalDevice physical_device;

    struct Core {
      VkDevice device;
      const VkAllocationCallbacks *allocator;
      #ifdef TRACY_ENABLE
        tracy::VkCtx *tracy_context;
      #endif

      struct Properties {
        VkPhysicalDeviceProperties basic;
        VkPhysicalDeviceMemoryProperties memory;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing;
      } properties;

      uint32_t queue_family_index;
    } core;

    VkQueue queue_present;
    VkQueue queue_work; // graphics, compute, transfer

    VkCommandPool tracy_setup_command_pool;

    lib::gfx::multi_alloc::Instance multi_alloc;

    struct Meshes {
      struct Item {
        engine::common::mesh::GPU_Data data;
      };

      std::vector<Item> items;
    } meshes;

    struct Textures {
      struct Item {
        engine::common::texture::GPU_Data data;
      };

      std::vector<Item> items;
    } textures;

    struct FullscreenQuad {
      lib::gfx::multi_alloc::StakeBuffer vertex_stake;
      size_t triangle_count;
    } fullscreen_quad;

    struct Prepass {
      VkRenderPass render_pass;
      VkPipeline pipeline;
    } prepass;

    struct GPass {
      VkDescriptorSetLayout descriptor_set_layout_frame;
      VkDescriptorSetLayout descriptor_set_layout_mesh;
      VkPipelineLayout pipeline_layout; // shared with prepass!
      VkRenderPass render_pass;
      VkPipeline pipeline;
      VkSampler sampler_albedo;
    } gpass;

    struct LPass {
      VkDescriptorSetLayout descriptor_set_layout_frame;
      VkDescriptorSetLayout descriptor_set_layout_directional_light;
      VkPipelineLayout pipeline_layout;
      VkRenderPass render_pass;
      VkPipeline pipeline_sun;
    } lpass;

    struct Finalpass {
      VkDescriptorSetLayout descriptor_set_layout;
      VkPipelineLayout pipeline_layout;
      VkPipeline pipeline;
      VkSampler sampler_lbuffer;
    } finalpass;
  } vulkan;

  struct ImguiContext {
    bool ready;
    // Dummy struct for future-proofing and demagic-ing.
    // ImGui::* and ImGui_ImplGlfw_* functions use a global singleton,
    // but if they took a parameter, we'd store that here.
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

struct SessionSetupData {
  // Note: this is not used anymore, but in the future in likely will.
  VkCommandPool command_pool;
  VkSemaphore semaphore_finished;
};
