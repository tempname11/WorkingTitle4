#pragma once
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <TracyVulkan.hpp>
#include <src/engine/common/texture.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/lib/debug_camera.hxx>

struct MetaTexturesKey {
  std::string path;
  VkFormat format;

  bool operator==(const MetaTexturesKey& other) const {
    return (true
      && path == other.path
      && format == other.format
    );
  }
};

namespace std {
  template <>
  struct hash<MetaTexturesKey> {
    size_t operator()(const MetaTexturesKey& key) const {
      return hash<std::string>()(key.path) ^ hash<VkFormat>()(key.format);
    }
  };
}

struct SessionData : lib::task::ParentResource {
  struct GLFW {
    bool ready;
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  using GuidCounter = lib::guid::Counter;
  GuidCounter guid_counter;

  struct UnfinishedYarns {
    std::mutex mutex;
    std::unordered_set<lib::Task *> set;
  } unfinished_yarns;

  struct Groups {
    enum struct Status {
      Loading,
      Ready
    };
     
    struct Item {
      Status status;
      std::string name;
    };

    std::unordered_map<lib::GUID, Item> items;
  } groups;

  struct Scene {
    struct Item {
      lib::GUID group_id;
      glm::mat4 transform;
      lib::GUID mesh_id;
      lib::GUID texture_albedo_id;
      lib::GUID texture_normal_id;
      lib::GUID texture_romeao_id;
    };

    std::vector<Item> items;
  } scene;

  struct MetaMeshes {
    std::unordered_map<std::string, lib::GUID> by_path;

    enum struct Status {
      Loading,
      Ready
    };

    struct Item {
      size_t ref_count;
      Status status;
      bool invalid;
      lib::Task *will_have_loaded;
      lib::Task *will_have_reloaded;
      std::string path;
    } item;

    std::unordered_map<lib::GUID, Item> items;
  } meta_meshes;

  struct MetaTextures {
    std::unordered_map<MetaTexturesKey, lib::GUID> by_key;

    enum struct Status {
      Loading,
      Ready
    };

    struct Item {
      size_t ref_count;
      Status status;
      lib::Task *will_have_loaded;
      std::string path;
      VkFormat format;
    } item;

    std::unordered_map<lib::GUID, Item> items;
  } meta_textures;

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

    using MultiAlloc = lib::gfx::multi_alloc::Instance;
    MultiAlloc multi_alloc;

    struct Meshes {
      using Item = engine::common::mesh::GPU_Data;
      std::unordered_map<lib::GUID, Item> items;
    } meshes;

    struct Textures {
      using Item = engine::common::texture::GPU_Data;
      std::unordered_map<lib::GUID, Item> items;
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

  using GPU_SignalSupport = lib::gpu_signal::Support;
  GPU_SignalSupport gpu_signal_support;

  struct Info {
    size_t worker_count;
  } info;

  struct State {
    bool show_imgui;
    bool show_imgui_window_demo;
    bool show_imgui_window_groups;
    bool show_imgui_window_meshes;
    bool is_fullscreen;
    lib::debug_camera::State debug_camera;
  } state;
};

struct SessionSetupData {
  // Note: this is not used anymore, but in the future in likely will.
  VkCommandPool command_pool;
  VkSemaphore semaphore_finished;
};
