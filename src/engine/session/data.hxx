#pragma once
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/engine/system/grup/data.hxx>
#include <src/engine/system/artline/data.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/common/ubo.hxx>
#include <src/engine/uploader.data.hxx>
#include <src/engine/blas_storage/data.hxx>
#include <src/engine/blas_storage/id.hxx>
#include "vulkan.data.hxx"

namespace engine::session {

struct Data : lib::task::ParentResource {
  struct GLFW {
    bool ready;
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  struct InflightGPU {
    static const size_t MAX_COUNT = 4;
    std::mutex mutex;
    lib::task::Task *signals[MAX_COUNT];
  } inflight_gpu;

  using GuidCounter = lib::guid::Counter;
  GuidCounter guid_counter;

  lib::Lifetime lifetime;

  struct Scene {
    struct Item {
      lib::GUID owner_id;
      glm::mat4 transform;
      lib::GUID mesh_id;
      lib::GUID texture_albedo_id;
      lib::GUID texture_normal_id;
      lib::GUID texture_romeao_id;
    };

    std::vector<Item> items;
  } scene;

  struct Grup {
    system::grup::Groups groups;
    system::grup::MetaMeshes meta_meshes;
    system::grup::MetaTextures meta_textures;
  } grup;

  system::artline::Data artline;

  Vulkan vulkan;

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
    bool ignore_glfw_events;
    bool show_imgui;
    bool show_imgui_window_demo;
    bool show_imgui_window_groups;
    bool show_imgui_window_meshes;
    bool show_imgui_window_textures;
    bool show_imgui_window_artline;
    bool show_imgui_window_gpu_memory;
    bool show_imgui_window_tools;
    bool show_imgui_window_flags;
    bool is_fullscreen;
    glm::vec2 taa_jitter_offset;
    glm::vec2 taa_jitter_offset_prev;
    float movement_speed;
    lib::debug_camera::State debug_camera;
    lib::debug_camera::State debug_camera_prev;
    glm::vec3 sun_position_xy;
    float sun_irradiance;
    float luminance_moving_average;
    float taa_distance;
    engine::common::ubo::Flags ubo_flags;
  } state;

  #ifdef ENGINE_DEVELOPER
    struct FrameControl {
      bool enabled;
      std::mutex mutex;
      size_t allowed_count;
      lib::Task *signal_allowed;
      lib::Task *signal_done;

      struct Directives {
          bool should_capture_screenshot;
          std::string screenshot_path;
      } directives;
    } frame_control;
  #endif
};

struct SetupData {
  // Note: this is not used anymore, but in the future in likely will.
  VkCommandPool command_pool;
  VkSemaphore semaphore_finished;
};

} // namespace
