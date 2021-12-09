#pragma once
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/flat32.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/lib/debug_camera.hxx>
/* :DeprecateGrup
#include <src/engine/system/grup/decl.hxx>
*/
#include <src/engine/system/artline/data.hxx>
#include <src/engine/system/entities/decl.hxx>
#include <src/engine/component.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/common/ubo.hxx>
#include <src/engine/component.hxx>
#include "data/inflight_gpu.hxx"

namespace engine::system::ode { struct Impl; }

namespace engine::session {

struct VulkanData;
struct Inflight_GPU_Data;

struct Data : lib::task::ParentResource {
  lib::allocator_t *init_allocator; // only used for init

  struct GLFW {
    bool ready;
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  Inflight_GPU_Data *inflight_gpu;
  lib::guid::Counter *guid_counter;

  lib::Lifetime lifetime;

  /* :DeprecateGrup
  struct Grup {
    system::grup::Groups *groups;
    system::grup::MetaMeshes *meta_meshes;
    system::grup::MetaTextures *meta_textures;
  } grup;
  */

  system::artline::Data artline;

  VulkanData *vulkan;

  struct ImguiContext {
    bool ready;
    // Dummy struct for future-proofing and demagic-ing.
    // ImGui::* and ImGui_ImplGlfw_* functions use a global singleton,
    // but if they took a parameter, we'd store that here.
  } imgui_context;

  lib::gpu_signal::Support *gpu_signal_support;

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

  system::entities::Impl *entities;
  system::ode::Impl *ode;

  struct Components {
    component::artline_model::storage_t artline_model;
    component::base_transform::storage_t base_transform;
  } components;

  #ifdef ENGINE_DEVELOPER
    struct FrameControl {
      bool enabled;
      lib::mutex_t mutex;
      size_t allowed_count;
      lib::Task *signal_allowed;
      lib::Task *signal_done;

      struct Directives {
        bool should_capture_screenshot;
        std::string screenshot_path;
      } directives;
    } *frame_control;
  #endif
};

struct SetupData {
  // Note: this is not used anymore, but in the future in likely will.
  VkCommandPool command_pool;
  VkSemaphore semaphore_finished;
};

} // namespace
