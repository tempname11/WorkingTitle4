#include <chrono>
#include <vector>
#include <unordered_set>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <TracyVulkan.hpp>
#include <imgui.h>
#include <tiny_gltf.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <src/embedded.hxx>
#include <src/lib/task.hxx>
#include <src/lib/debug_camera.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/lib/gfx/mesh.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/global.hxx>

// #define ENGINE_DEBUG_TASK_THREADS 1
// #define ENGINE_DEBUG_ARTIFICIAL_DELAY 33ms

namespace task = lib::task;
namespace usage = lib::usage;

namespace mesh {
  T05 read_t05_file(const char *filename) {
    auto file = fopen(filename, "rb");
    assert(file != nullptr);
    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);
    auto buffer = (uint8_t *) malloc(size);
    fread((void *) buffer, 1, size, file);
    assert(ferror(file) == 0);
    fclose(file);
    auto triangle_count = *((uint32_t*) buffer);
    assert(size == sizeof(uint32_t) + sizeof(VertexT05) * 3 * triangle_count);
    return {
      .buffer = buffer,
      .triangle_count = triangle_count,
      // three vertices per triangle
      .vertices = ((VertexT05 *) (buffer + sizeof(uint32_t))),
    };
  }

  void deinit_t05(T05 *it) {
    free(it->buffer);
  }
}

namespace example {
  struct VS_UBO {
    glm::mat4 projection;
    glm::mat4 view;
  };

  struct FS_UBO {
    alignas(16) glm::vec3 camera_position;

    // light
    alignas(16) glm::vec3 light_position;
    alignas(16) glm::vec3 light_intensity;

    // material
    alignas(16) glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao;
  };
}

const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

const int DEFAULT_WINDOW_WIDTH = 1280;
const int DEFAULT_WINDOW_HEIGHT = 720;

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT type,
  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
  void *user_data
) {
  if (
    severity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT &&
    type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
  ) {
    return VK_FALSE;
  }
  LOG("{}", callback_data->pMessage);
  return VK_FALSE;
}

struct CommandPool2 {
  std::mutex mutex;
  std::vector<VkCommandPool> pools;
};

VkCommandPool command_pool_2_borrow(CommandPool2 *pool2) {
  std::scoped_lock lock(pool2->mutex);
  assert(pool2->pools.size() > 0);
  auto pool = pool2->pools.back();
  pool2->pools.pop_back();
  return pool;
}

void command_pool_2_return(CommandPool2 *pool2, VkCommandPool pool) {
  std::scoped_lock lock(pool2->mutex);
  pool2->pools.push_back(pool);
}

struct SessionData : task::ParentResource {
  struct GLFW {
    GLFWwindow *window;
    glm::ivec2 last_window_position;
    glm::ivec2 last_window_size;
    glm::vec2 last_known_mouse_cursor_position;
  } glfw;

  struct Vulkan : task::ParentResource {
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
      lib::gfx::multi_alloc::StakeBuffer vertex_stake;
      VkDescriptorSetLayout descriptor_set_layout;
      VkDescriptorSet descriptor_set;
      VkPipelineLayout pipeline_layout;
      size_t triangle_count;
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

struct RenderingData : task::ParentResource {
  struct Presentation {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkSemaphore> image_acquired;
    std::vector<VkSemaphore> image_rendered;
    uint32_t latest_image_index;
  } presentation;

  struct PresentationFailureState {
    std::mutex mutex;
    bool failure;
  } presentation_failure_state;

  struct SwapchainDescription {
    uint8_t image_count;
    VkExtent2D image_extent;
    VkFormat image_format;
  } swapchain_description;

  struct InflightGPU {
    std::mutex mutex;
    std::vector<task::Task *> signals;
  } inflight_gpu;

  struct ImguiBackend {
    // ImGui_ImplVulkan_*
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool setup_command_pool;
    VkSemaphore setup_semaphore;
  } imgui_backend; // @Note: should probably be in SessionData!

  struct FrameInfo {
    uint64_t timestamp_ns;
    uint64_t elapsed_ns;

    uint64_t number;
    uint64_t timeline_semaphore_value;
    uint8_t inflight_index;
  } latest_frame;

  typedef std::vector<CommandPool2> CommandPools;
  CommandPools command_pools;

  VkSemaphore example_finished_semaphore;
  VkSemaphore imgui_finished_semaphore;
  VkSemaphore frame_rendered_semaphore;

  lib::gfx::multi_alloc::Instance multi_alloc;

  struct Example {
    uint32_t vs_ubo_aligned_size;
    uint32_t fs_ubo_aligned_size;
    uint32_t total_ubo_aligned_size;
    lib::gfx::multi_alloc::StakeBuffer uniform_stake;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    std::vector<lib::gfx::multi_alloc::StakeImage> image_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> depth_stakes;
    std::vector<VkImageView> image_views;
    std::vector<VkImageView> depth_views;
    std::vector<VkFramebuffer> framebuffers;
  } example;
};

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

void defer(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<task::Task> task
) {
  ZoneScoped;
  task::inject(ctx->runner, { task.ptr });
}

void defer_many(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Full<std::vector<task::Task *>> tasks
) {
  ZoneScoped;
  task::inject(ctx->runner, std::move(*tasks));
  delete tasks.ptr;
}

void rendering_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Some<SessionData> session,
  usage::Full<RenderingData> data
) {
  ZoneScoped;
  auto vulkan = &session->vulkan;
  { ZoneScopedN("example");
    for (auto framebuffer : data->example.framebuffers) {
      vkDestroyFramebuffer(
        vulkan->core.device,
        framebuffer,
        vulkan->core.allocator
      );
    }
    for (auto image_view : data->example.image_views) {
      vkDestroyImageView(
        vulkan->core.device,
        image_view,
        vulkan->core.allocator
      );
    }
    for (auto image_view : data->example.depth_views) {
      vkDestroyImageView(
        vulkan->core.device,
        image_view,
        vulkan->core.allocator
      );
    }
    vkDestroyRenderPass(
      vulkan->core.device,
      data->example.render_pass,
      vulkan->core.allocator
    );
    vkDestroyPipeline(
      vulkan->core.device,
      data->example.pipeline,
      vulkan->core.allocator
    );
  }
  lib::gfx::multi_alloc::deinit(
    &data->multi_alloc,
    vulkan->core.device,
    vulkan->core.allocator
  );
  vkDestroyRenderPass(
    vulkan->core.device,
    data->imgui_backend.render_pass,
    vulkan->core.allocator
  );
  for (auto framebuffer : data->imgui_backend.framebuffers) {
    vkDestroyFramebuffer(
      vulkan->core.device,
      framebuffer,
      vulkan->core.allocator
    );
  }
  ImGui_ImplVulkan_Shutdown();
  for (auto &pool2 : data->command_pools) {
    for (auto pool : pool2.pools) {
      vkDestroyCommandPool(
        vulkan->core.device,
        pool,
        vulkan->core.allocator
      );
    }
  }
  for (size_t i = 0; i < data->swapchain_description.image_count; i++) {
    vkDestroySemaphore(
      vulkan->core.device,
      data->presentation.image_acquired[i],
      vulkan->core.allocator
    );
    vkDestroySemaphore(
      vulkan->core.device,
      data->presentation.image_rendered[i],
      vulkan->core.allocator
    );
  }
  vkDestroySemaphore(
    vulkan->core.device,
    data->example_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->imgui_finished_semaphore,
    vulkan->core.allocator
  );
  vkDestroySemaphore(
    vulkan->core.device,
    data->frame_rendered_semaphore,
    vulkan->core.allocator
  );
  vkDestroySwapchainKHR(
    vulkan->core.device,
    data->presentation.swapchain,
    vulkan->core.allocator
  );
  delete data.ptr;
  ctx->changed_parents = {
    { .ptr = data.ptr, .children = {} }
  };
  task::signal(ctx->runner, session_iteration_yarn_end.ptr);
}

void rendering_frame_handle_window_events(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData::GLFW> glfw,
  usage::Full<SessionData::State> session_state,
  usage::Full<UpdateData> update
) {
  ZoneScoped;
  auto user_data = new GlfwUserData {
    .state = session_state.ptr,
  };
  glfwSetWindowUserPointer(glfw->window, user_data);
  glfwPollEvents();
  glfwSetWindowUserPointer(glfw->window, nullptr);

  auto cursor_delta = glm::vec2(0.0);
  auto is_focused = glfwGetWindowAttrib(glfw->window, GLFW_FOCUSED);
  { // update mouse position
    double x, y;
    int w, h;
    glfwGetCursorPos(glfw->window, &x, &y);
    glfwGetWindowSize(glfw->window, &w, &h);
    if (w > 0 && h > 0) {
      auto new_position = 2.0f * glm::vec2(x / w, y / h) - 1.0f;
      if (is_focused
        && !isnan(glfw->last_known_mouse_cursor_position.x)
        && !isnan(glfw->last_known_mouse_cursor_position.y)
      ) {
        cursor_delta = new_position - glfw->last_known_mouse_cursor_position;
      }
      glfw->last_known_mouse_cursor_position = new_position;
    } else {
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    }
  }
  { ZoneScopedN("fullscreen");
    // @Incomplete
    auto monitor = glfwGetWindowMonitor(glfw->window);
    if (monitor == nullptr && session_state->is_fullscreen) {
      // save last position
      glfwGetWindowPos(glfw->window, &glfw->last_window_position.x, &glfw->last_window_position.y);
      glfwGetWindowSize(glfw->window, &glfw->last_window_size.x, &glfw->last_window_size.y);

      monitor = glfwGetPrimaryMonitor();
      auto mode = glfwGetVideoMode(monitor);

      glfwSetWindowMonitor(
        glfw->window, monitor,
        0,
        0,
        mode->width,
        mode->height,
        mode->refreshRate
      );
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    } else if (monitor != nullptr && !session_state->is_fullscreen) {
      glfwSetWindowMonitor(
        glfw->window, nullptr,
        glfw->last_window_position.x, 
        glfw->last_window_position.y, 
        glfw->last_window_size.x,
        glfw->last_window_size.y,
        GLFW_DONT_CARE
      );
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    }
  }
  { ZoneScopedN("cursor_mode");
    int desired_cursor_mode = (
      !is_focused || session_state->show_imgui
        ? GLFW_CURSOR_NORMAL
        : GLFW_CURSOR_DISABLED
    );
    int actual_cursor_mode = glfwGetInputMode(glfw->window, GLFW_CURSOR);
    if (desired_cursor_mode != actual_cursor_mode) {
      glfwSetInputMode(glfw->window, GLFW_CURSOR, desired_cursor_mode);
    }
  }
  { ZoneScopedN("update_data");
    auto it = &update->debug_camera_input;
    *it = {};
    if (!session_state->show_imgui) {
      it->cursor_position_delta = cursor_delta;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_W) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_UP) == GLFW_PRESS)
    ) {
      it->y_pos = true;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_S) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_DOWN) == GLFW_PRESS)
    ) {
      it->y_neg = true;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_D) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    ) {
      it->x_pos = true;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_A) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_LEFT) == GLFW_PRESS)
    ) {
      it->x_neg = true;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_E) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
    ) {
      it->z_pos = true;
    }
    if (0
      || (glfwGetKey(glfw->window, GLFW_KEY_Q) == GLFW_PRESS)
      || (glfwGetKey(glfw->window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
    ) {
      it->z_neg = true;
    }
  }
}

void rendering_frame_update(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<UpdateData> update,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<SessionData::State> session_state
) {
  // @Note: part of update currently happens in glfwPollEvents
  ZoneScoped;
  double elapsed_sec = double(frame_info->elapsed_ns) / (1000 * 1000 * 1000);
  if (elapsed_sec > 0) {
    lib::debug_camera::update(
      &session_state->debug_camera,
      &update->debug_camera_input,
      elapsed_sec
    );
  }
}

void signal_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::Full<uint8_t> inflight_index_saved
) {
  std::scoped_lock lock(inflight_gpu->mutex);
  assert(inflight_gpu->signals[*inflight_index_saved] != nullptr);
  inflight_gpu->signals[*inflight_index_saved] = nullptr;
  delete inflight_index_saved.ptr;
}

void rendering_frame_setup_gpu_signal(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<lib::gpu_signal::Support> gpu_signal_support,
  usage::Full<VkSemaphore> frame_rendered_semaphore,
  usage::Full<RenderingData::InflightGPU> inflight_gpu,
  usage::Some<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  // don't use inflight_gpu->mutex, since out signal slot is currently unusedk
  assert(inflight_gpu->signals[frame_info->inflight_index] == nullptr);
  auto signal = lib::gpu_signal::create(
    gpu_signal_support.ptr,
    core->device,
    *frame_rendered_semaphore,
    frame_info->timeline_semaphore_value
  );
  auto inflight_index_saved = new uint8_t(frame_info->inflight_index); // frame_info will not be around!
  auto task_cleanup = task::create(signal_cleanup, inflight_gpu.ptr, inflight_index_saved);
  auto task_deferred_cleanup = task::create(defer, task_cleanup);
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    inflight_gpu->signals[frame_info->inflight_index] = task_cleanup; // not the signal itself, on purpose
  }
  lib::task::inject(ctx->runner, {
    task_deferred_cleanup
  }, {
    .new_dependencies = {
      { signal, task_deferred_cleanup }
    }
  });
}

void rendering_frame_reset_pools(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  auto &pool2 = (*command_pools)[frame_info->inflight_index];
  {
    std::scoped_lock lock(pool2.mutex);
    for (auto pool : pool2.pools) {
      vkResetCommandPool(
        core->device,
        pool,
        0
      );
    }
  }
}

void rendering_frame_example_prepare_uniforms(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<SessionData::State> session_state, // too broad
  usage::Full<RenderingData::Example> example_r
) {
  ZoneScoped;
  auto projection = lib::gfx::utilities::get_projection(
    float(swapchain_description->image_extent.width)
      / swapchain_description->image_extent.height
  );
  auto light_position = glm::vec3(3.0f, 5.0f, 2.0f);
  auto light_intensity = glm::vec3(1000.0f, 1000.0f, 1000.0f);
  auto albedo = glm::vec3(1.0, 1.0, 1.0);
  const example::VS_UBO vs = {
    .projection = projection,
    .view = lib::debug_camera::to_view_matrix(&session_state->debug_camera),
  };
  const example::FS_UBO fs = {
    .camera_position = session_state->debug_camera.position,
    .light_position = light_position,
    .light_intensity = light_intensity,
    .albedo = glm::vec3(1.0f, 1.0f, 1.0f),
    .metallic = 1.0f,
    .roughness = 0.5f,
    .ao = 0.1f,
  };

  void * data;
  vkMapMemory(
    core->device,
    example_r->uniform_stake.memory,
    example_r->uniform_stake.offset
      + frame_info->inflight_index * example_r->total_ubo_aligned_size,
    example_r->total_ubo_aligned_size,
    0, &data
  );
  memcpy(data, &vs, sizeof(example::VS_UBO));
  memcpy((uint8_t*) data + example_r->vs_ubo_aligned_size, &fs, sizeof(example::FS_UBO));
  vkUnmapMemory(core->device, example_r->uniform_stake.memory);
}

void rendering_frame_example_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<RenderingData::Example> example_r,
  usage::Some<SessionData::Vulkan::Example> example_s,
  usage::Full<ExampleData> data
) {
  ZoneScoped;
  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  auto r = example_r.ptr;
  auto s = example_s.ptr;
  VkCommandPool pool = command_pool_2_borrow(pool2);
  VkCommandBuffer cmd;
  { // cmd
    VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    auto result = vkAllocateCommandBuffers(
      core->device,
      &info,
      &cmd
    );
    assert(result == VK_SUCCESS);
  }
  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }
  { TracyVkZone(core->tracy_context, cmd, "example_render");
    VkClearValue clear_values[] = {
      {0.0f, 0.0f, 0.0f, 0.0f},
      {1.0f, 0.0f},
    };
    VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = r->render_pass,
      .framebuffer = r->framebuffers[frame_info->inflight_index],
      .renderArea = {
        .offset = {0, 0},
        .extent = swapchain_description->image_extent,
      },
      .clearValueCount = sizeof(clear_values) / sizeof(*clear_values),
      .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r->pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &s->vertex_stake.buffer, &offset);
    uint32_t dynamicOffsets[2] = {
      frame_info->inflight_index * r->total_ubo_aligned_size,
      frame_info->inflight_index * r->total_ubo_aligned_size + r->vs_ubo_aligned_size,
    };
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
      s->pipeline_layout,
      0, 1, &s->descriptor_set,
      2, dynamicOffsets
    );
    vkCmdDraw(cmd, s->triangle_count * 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd);
  }
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}

void rendering_frame_example_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Some<VkSemaphore> example_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ExampleData> data
) {
  ZoneScoped;
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = example_finished_semaphore.ptr,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
  delete data.ptr;
}

void rendering_frame_imgui_new_frame(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
) {
  // main thread because of glfw usage
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void rendering_frame_imgui_populate(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<SessionData::ImguiContext> imgui,
  usage::Some<SessionData::State> state
) {
  if (state->show_imgui) {
    ImGui::ShowDemoWindow(nullptr);
  }
}

void rendering_frame_imgui_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::ImguiContext> imgui_context,
  usage::Full<RenderingData::ImguiBackend> imgui_backend,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
) {
  ZoneScoped;
  ImGui::Render();
  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  VkCommandPool pool = command_pool_2_borrow(pool2);
  VkCommandBuffer cmd;
  { // cmd
    VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    auto result = vkAllocateCommandBuffers(
      core->device,
      &info,
      &cmd
    );
    assert(result == VK_SUCCESS);
  }
  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }
  auto drawData = ImGui::GetDrawData();
  { // begin render pass
    auto begin_info = VkRenderPassBeginInfo {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = imgui_backend->render_pass,
      .framebuffer = imgui_backend->framebuffers[frame_info->inflight_index],
      .renderArea = {
        .extent = swapchain_description->image_extent,
      },
      .clearValueCount = 0,
      .pClearValues = nullptr,
    };
    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
  }
  ImGui_ImplVulkan_RenderDrawData(
    drawData,
    cmd
  );
  vkCmdEndRenderPass(cmd);
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}

void rendering_frame_imgui_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Some<VkSemaphore> example_finished_semaphore,
  usage::Some<VkSemaphore> imgui_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ImguiData> data
) {
  ZoneScoped;
  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .waitSemaphoreValueCount = 1,
    .pWaitSemaphoreValues = &frame_info->timeline_semaphore_value,
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = example_finished_semaphore.ptr,
    .pWaitDstStageMask = &wait_stage,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = imgui_finished_semaphore.ptr,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
  delete data.ptr;
}

void rendering_frame_acquire(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  uint32_t image_index;
  auto result = vkAcquireNextImageKHR(
    core->device,
    presentation->swapchain,
    0,
    presentation->image_acquired[frame_info->inflight_index],
    VK_NULL_HANDLE,
    &presentation->latest_image_index
  );
  if (result == VK_SUBOPTIMAL_KHR) {
    // not sure what to do with this.
    // log and treat as success.
    LOG("vkAcquireNextImageKHR: VK_SUBOPTIMAL_KHR");
  } else if (false
    || result == VK_ERROR_OUT_OF_DATE_KHR
    || result == VK_ERROR_SURFACE_LOST_KHR
    || result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
  ) {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_failure_state->failure = true;
  } else {
    assert(result == VK_SUCCESS);
  }
}

void rendering_frame_compose_render(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::CommandPools> command_pools,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Some<RenderingData::Example> example_r,
  usage::Full<ComposeData> data
) {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      return;
    }
  }
  auto pool2 = &(*command_pools)[frame_info->inflight_index];
  VkCommandPool pool = command_pool_2_borrow(pool2);
  VkCommandBuffer cmd;
  { // cmd
    VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    auto result = vkAllocateCommandBuffers(
      core->device,
      &info,
      &cmd
    );
    assert(result == VK_SUCCESS);
  }
  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }
  { TracyVkZone(core->tracy_context, cmd, "compose_render");
    { // barrier, prepare acquired image to copy
      VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0, // wait for nothing
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = presentation->swapchain_images[presentation->latest_image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };
      vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // wait for nothing
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );
    }
    { // copy image
      auto region = VkImageCopy {
        .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
        },
        .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
        },
        .extent = {
          .width = swapchain_description->image_extent.width,
          .height = swapchain_description->image_extent.height,
          .depth = 1,
        },
      };
      vkCmdCopyImage(
        cmd,
        example_r->image_stakes[frame_info->inflight_index].image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        presentation->swapchain_images[presentation->latest_image_index],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
      );
    }
    { // barrier for presentation
      // @Note:
      // https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
      // "A PRACTICAL BOTTOM_OF_PIPE EXAMPLE" suggests we can
      // use dstStageMask = BOTTOM_OF_PIPE, dstAccessMask = 0
      VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = 0, // see above
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = presentation->swapchain_images[presentation->latest_image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };
      vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // see above
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );
    }
  }
  TracyVkCollect(core->tracy_context, cmd);
  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }
  command_pool_2_return(pool2, pool);
  data->cmd = cmd;
}

void rendering_frame_compose_submit(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<VkQueue> queue_work,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<VkSemaphore> frame_rendered_semaphore,
  usage::Some<VkSemaphore> imgui_finished_semaphore,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<ComposeData> data
) {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      // we still need to signal that gpu work is over.
      // so do a dummy submit that has no commands,
      // just "links" the semaphores
      auto timeline_info = VkTimelineSemaphoreSubmitInfo {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &frame_info->timeline_semaphore_value,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &frame_info->timeline_semaphore_value,
      };
      VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = imgui_finished_semaphore.ptr,
        .pWaitDstStageMask = &wait_stage,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = frame_rendered_semaphore.ptr,
      };
      auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
      return;
    }
  }
  uint64_t zero = 0;
  VkSemaphore wait_semaphores[] = {
    *imgui_finished_semaphore,
    presentation->image_acquired[frame_info->inflight_index]
  };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
  };
  uint64_t wait_values[] = {
    frame_info->timeline_semaphore_value,
    zero,
  };
  VkSemaphore signal_semaphores[] = {
    *frame_rendered_semaphore,
    presentation->image_rendered[frame_info->inflight_index],
  };
  uint64_t signal_values[] = {
    frame_info->timeline_semaphore_value,
    zero,
  };
  auto timeline_info = VkTimelineSemaphoreSubmitInfo {
    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    .waitSemaphoreValueCount = sizeof(wait_values) / sizeof(*wait_values),
    .pWaitSemaphoreValues = wait_values,
    .signalSemaphoreValueCount = sizeof(signal_values) / sizeof(*signal_values),
    .pSignalSemaphoreValues = signal_values,
  };
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = &timeline_info,
    .waitSemaphoreCount = sizeof(wait_semaphores) / sizeof(*wait_semaphores),
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &data->cmd,
    .signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(*signal_semaphores),
    .pSignalSemaphores = signal_semaphores,
  };
  auto result = vkQueueSubmit(*queue_work, 1, &submit_info, VK_NULL_HANDLE);
  assert(result == VK_SUCCESS);
  delete data.ptr;
}

void rendering_frame_present(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<RenderingData::Presentation> presentation,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Some<RenderingData::FrameInfo> frame_info,
  usage::Full<VkQueue> queue_present
) {
  ZoneScoped;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    if (presentation_failure_state->failure) {
      return;
    }
  }
  VkPresentInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &presentation->image_rendered[frame_info->inflight_index],
    .swapchainCount = 1,
    .pSwapchains = &presentation->swapchain,
    .pImageIndices = &presentation->latest_image_index,
  };
  auto result = vkQueuePresentKHR(*queue_present, &info);
  if (result == VK_SUBOPTIMAL_KHR) {
    // not sure what to do with this.
    // log and treat as success.
    LOG("vkQueuePresentKHR: VK_SUBOPTIMAL_KHR");
  } else if (false
    || result == VK_ERROR_OUT_OF_DATE_KHR
    || result == VK_ERROR_SURFACE_LOST_KHR
    || result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
  ) {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_failure_state->failure = true;
  } else {
    assert(result == VK_SUCCESS);
  }
}

void rendering_frame_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  delete frame_info.ptr;
}

void rendering_has_finished(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<task::Task> rendering_yarn_end
) {
  ZoneScoped;
  lib::task::signal(ctx->runner, rendering_yarn_end.ptr);
}

void rendering_frame(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<task::Task> rendering_yarn_end,
  usage::None<SessionData> session,
  usage::None<RenderingData> data,
  usage::Some<SessionData::GLFW> glfw,
  usage::Some<RenderingData::PresentationFailureState> presentation_failure_state,
  usage::Full<RenderingData::FrameInfo> latest_frame,
  usage::Some<RenderingData::SwapchainDescription> swapchain_description,
  usage::Some<RenderingData::InflightGPU> inflight_gpu
) {
  ZoneScopedC(0xFF0000);
  FrameMark;
  #ifdef ENGINE_DEBUG_ARTIFICIAL_DELAY
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(ENGINE_DEBUG_ARTIFICIAL_DELAY);
    }
  #endif
  bool presentation_has_failed;
  {
    std::scoped_lock lock(presentation_failure_state->mutex);
    presentation_has_failed = presentation_failure_state->failure;
  }
  bool should_stop = glfwWindowShouldClose(glfw->window);
  if (should_stop || presentation_has_failed) {
    std::scoped_lock lock(inflight_gpu->mutex);
    auto task_has_finished = task::create(rendering_has_finished, rendering_yarn_end.ptr);
    task::Auxiliary aux;
    for (auto signal : inflight_gpu->signals) {
      aux.new_dependencies.push_back({ signal, task_has_finished });
    }
    task::inject(ctx->runner, { task_has_finished }, std::move(aux));
    return;
  }
  { // timing
    using namespace std::chrono_literals;
    uint64_t new_timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch() / 1ns;
    if (new_timestamp_ns > latest_frame->timestamp_ns) {
      latest_frame->elapsed_ns = new_timestamp_ns - latest_frame->timestamp_ns;
    } else {
      latest_frame->elapsed_ns = 0;
    }
    latest_frame->timestamp_ns = new_timestamp_ns;
  }
  latest_frame->number++;
  latest_frame->timeline_semaphore_value = latest_frame->number + 1;
  latest_frame->inflight_index = (
    latest_frame->number % swapchain_description->image_count
  );
  auto frame_info = new RenderingData::FrameInfo(*latest_frame);
  auto compose_data = new ComposeData;
  auto example_data = new ExampleData;
  auto imgui_data = new ImguiData;
  auto update_data = new UpdateData;
  auto frame_tasks = new std::vector<task::Task *>({
    task::create(
      rendering_frame_handle_window_events,
      &session->glfw,
      &session->state,
      update_data
    ),
    task::create(
      rendering_frame_update,
      update_data,
      frame_info,
      &session->state
    ),
    task::create(
      rendering_frame_setup_gpu_signal,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &data->frame_rendered_semaphore,
      &data->inflight_gpu,
      frame_info
    ),
    task::create(
      rendering_frame_reset_pools,
      &session->vulkan.core,
      &data->command_pools,
      frame_info
    ),
    task::create(
      rendering_frame_acquire,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info
    ),
    task::create(
      rendering_frame_example_prepare_uniforms,
      &session->vulkan.core,
      &data->swapchain_description,
      frame_info,
      &session->state,
      &data->example
    ),
    task::create(
      rendering_frame_example_render,
      &session->vulkan.core,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->example,
      &session->vulkan.example,
      example_data
    ),
    task::create(
      rendering_frame_example_submit,
      &session->vulkan.queue_work,
      &data->example_finished_semaphore,
      frame_info,
      example_data
    ),
    task::create(
      rendering_frame_imgui_new_frame,
      &session->imgui_context,
      &data->imgui_backend
    ),
    task::create(
      rendering_frame_imgui_populate,
      &session->imgui_context,
      &session->state
    ),
    task::create(
      rendering_frame_imgui_render,
      &session->vulkan.core,
      &session->imgui_context,
      &data->imgui_backend,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      imgui_data
    ),
    task::create(
      rendering_frame_imgui_submit,
      &session->vulkan.queue_work,
      &data->example_finished_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      imgui_data
    ),
    task::create(
      rendering_frame_compose_render,
      &session->vulkan.core,
      &data->presentation,
      &data->presentation_failure_state,
      &data->swapchain_description,
      &data->command_pools,
      frame_info,
      &data->example,
      compose_data
    ),
    task::create(
      rendering_frame_compose_submit,
      &session->vulkan.queue_work,
      &data->presentation,
      &data->presentation_failure_state,
      &data->frame_rendered_semaphore,
      &data->imgui_finished_semaphore,
      frame_info,
      compose_data
    ),
    task::create(
      rendering_frame_present,
      &data->presentation,
      &data->presentation_failure_state,
      frame_info,
      &session->vulkan.queue_present
    ),
    task::create(
      rendering_frame_cleanup,
      frame_info
    ),
    task::create(
      rendering_frame,
      rendering_yarn_end.ptr,
      session.ptr,
      data.ptr,
      glfw.ptr,
      presentation_failure_state.ptr,
      latest_frame.ptr,
      swapchain_description.ptr,
      inflight_gpu.ptr
    ),
  });
  auto task_defer = task::create(defer_many, frame_tasks);
  { // inject under inflight mutex
    std::scoped_lock lock(inflight_gpu->mutex);
    auto signal = inflight_gpu->signals[latest_frame->inflight_index];
    task::inject(ctx->runner, {
      task_defer,
    }, {
      .new_dependencies = { { signal, task_defer } },
    });
  }
}

void rendering_imgui_setup_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
) {
  ImGui_ImplVulkan_DestroyFontUploadObjects();
  vkDestroyCommandPool(
    core->device,
    imgui_backend->setup_command_pool,
    core->allocator
  );
  vkDestroySemaphore(
    core->device,
    imgui_backend->setup_semaphore,
    core->allocator
  );
}

void session_iteration_try_rendering(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  usage::Full<task::Task> session_iteration_yarn_end,
  usage::Full<SessionData> session
) {
  ZoneScoped;
  VkCommandBuffer cmd_imgui_setup = VK_NULL_HANDLE;
  task::Task* signal_imgui_setup_finished = nullptr;
  auto vulkan = &session->vulkan;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  { // get capabilities
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      vulkan->physical_device,
      vulkan->window_surface,
      &surface_capabilities
    );
    assert(result == VK_SUCCESS);
  }

  if (
    surface_capabilities.currentExtent.width == 0 ||
    surface_capabilities.currentExtent.height == 0
  ) {
    lib::task::signal(ctx->runner, session_iteration_yarn_end.ptr);
    return;
  }
  
  auto rendering = new RenderingData;
  // how many images are in swapchain?
  uint32_t swapchain_image_count;
  { ZoneScopedN(".presentation");
    VkSwapchainCreateInfoKHR swapchain_create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = vulkan->window_surface,
      .minImageCount = surface_capabilities.minImageCount,
      .imageFormat = SWAPCHAIN_FORMAT,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = surface_capabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = surface_capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
    };
    {
      auto result = vkCreateSwapchainKHR(
        vulkan->core.device,
        &swapchain_create_info,
        vulkan->core.allocator,
        &rendering->presentation.swapchain
      );
      assert(result == VK_SUCCESS);
    }
    {
      auto result = vkGetSwapchainImagesKHR(
        session->vulkan.core.device,
        rendering->presentation.swapchain,
        &swapchain_image_count,
        nullptr
      );
      assert(result == VK_SUCCESS);
    }
    rendering->presentation.swapchain_images.resize(swapchain_image_count);
    {
      auto result = vkGetSwapchainImagesKHR(
        session->vulkan.core.device,
        rendering->presentation.swapchain,
        &swapchain_image_count,
        rendering->presentation.swapchain_images.data()
      );
      assert(result == VK_SUCCESS);
    }
    rendering->presentation.image_acquired.resize(swapchain_image_count);
    rendering->presentation.image_rendered.resize(swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
      VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      {
        auto result = vkCreateSemaphore(
          session->vulkan.core.device,
          &info,
          session->vulkan.core.allocator,
          &rendering->presentation.image_acquired[i]
        );
        assert(result == VK_SUCCESS);
      }
      {
        auto result = vkCreateSemaphore(
          session->vulkan.core.device,
          &info,
          session->vulkan.core.allocator,
          &rendering->presentation.image_rendered[i]
        );
        assert(result == VK_SUCCESS);
      }
    }
    rendering->presentation.latest_image_index = uint32_t(-1);
  }
  rendering->presentation_failure_state.failure = false;
  rendering->swapchain_description.image_count = checked_integer_cast<uint8_t>(swapchain_image_count);
  rendering->swapchain_description.image_extent = surface_capabilities.currentExtent;
  rendering->swapchain_description.image_format = SWAPCHAIN_FORMAT;
  rendering->inflight_gpu.signals.resize(swapchain_image_count, nullptr);
  rendering->latest_frame.timestamp_ns = 0;
  rendering->latest_frame.elapsed_ns = 0;
  rendering->latest_frame.number = uint64_t(-1);
  rendering->latest_frame.inflight_index = uint8_t(-1);
  { ZoneScopedN(".command_pools");
    rendering->command_pools = std::vector<CommandPool2>(swapchain_image_count);
    for (size_t i = 0; i < swapchain_image_count; i++) {
      rendering->command_pools[i].pools.resize(session->info.worker_count);
      for (size_t j = 0; j < session->info.worker_count; j++) {
        VkCommandPool pool;
        {
          VkCommandPoolCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = session->vulkan.queue_family_index,
          };
          auto result = vkCreateCommandPool(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &pool
          );
          assert(result == VK_SUCCESS);
        }
        rendering->command_pools[i].pools[j] = pool;
      }
    }
  }
  { ZoneScopedN(".frame_rendered_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->frame_rendered_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  { ZoneScopedN(".example_finished_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->example_finished_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  { ZoneScopedN(".imgui_finished_semaphore");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    auto result = vkCreateSemaphore(
      session->vulkan.core.device,
      &create_info,
      session->vulkan.core.allocator,
      &rendering->imgui_finished_semaphore
    );
    assert(result == VK_SUCCESS);
  }
  rendering->example.vs_ubo_aligned_size = lib::gfx::utilities::aligned_size(
    sizeof(example::VS_UBO),
    session->vulkan.properties.basic.limits.minUniformBufferOffsetAlignment
  );
  rendering->example.fs_ubo_aligned_size = lib::gfx::utilities::aligned_size(
    sizeof(example::FS_UBO),
    session->vulkan.properties.basic.limits.minUniformBufferOffsetAlignment
  );
  rendering->example.total_ubo_aligned_size = (
    rendering->example.vs_ubo_aligned_size +
    rendering->example.fs_ubo_aligned_size
  );
  { ZoneScopedN(".multi_alloc");
    std::vector<lib::gfx::multi_alloc::Claim> claims;
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = (
            rendering->example.total_ubo_aligned_size
            * rendering->swapchain_description.image_count
          ),
          .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
      },
      .memory_property_flags = VkMemoryPropertyFlagBits(0
        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      .p_stake_buffer = &rendering->example.uniform_stake,
    });
    rendering->example.image_stakes.resize(
      rendering->swapchain_description.image_count
    );
    rendering->example.depth_stakes.resize(
      rendering->swapchain_description.image_count
    );
    for (auto &stake : rendering->example.image_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = rendering->swapchain_description.image_format,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->example.depth_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = DEPTH_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    lib::gfx::multi_alloc::init(
      &rendering->multi_alloc,
      std::move(claims),
      session->vulkan.core.device,
      session->vulkan.core.allocator,
      &session->vulkan.properties.basic,
      &session->vulkan.properties.memory
    );
  }
  { ZoneScopedN(".example");
    { ZoneScopedN("update_descriptor_set");
      VkDescriptorBufferInfo vs_info = {
        .buffer = rendering->example.uniform_stake.buffer,
        .offset = 0,
        .range = sizeof(example::VS_UBO),
      };
      VkDescriptorBufferInfo fs_info = {
        .buffer = rendering->example.uniform_stake.buffer,
        .offset = 0,
        .range = sizeof(example::FS_UBO),
      };
      VkWriteDescriptorSet writes[] = {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = session->vulkan.example.descriptor_set,
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          .pBufferInfo = &vs_info,
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = session->vulkan.example.descriptor_set,
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          .pBufferInfo = &fs_info,
        },
      };
      vkUpdateDescriptorSets(
        session->vulkan.core.device,
        sizeof(writes) / sizeof(*writes), writes,
        0, nullptr
      );
    }
    { ZoneScopedN(".render_pass");
      VkAttachmentDescription attachment_descriptions[] = {
        {
          .format = rendering->swapchain_description.image_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
          .format = DEPTH_FORMAT,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
      };
      VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription subpass_description = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
      };
      VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof(attachment_descriptions) / sizeof(*attachment_descriptions),
        .pAttachments = attachment_descriptions,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
      };
      auto result = vkCreateRenderPass(
        vulkan->core.device,
        &create_info,
        vulkan->core.allocator,
        &rendering->example.render_pass
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".pipeline");
      VkShaderModule module_frag = VK_NULL_HANDLE;
      { ZoneScopedN("module_frag");
        VkShaderModuleCreateInfo info = {
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = embedded_example_rs_frag_len,
          .pCode = (const uint32_t*) embedded_example_rs_frag,
        };
        auto result = vkCreateShaderModule(
          vulkan->core.device,
          &info,
          vulkan->core.allocator,
          &module_frag
        );
        assert(result == VK_SUCCESS);
      }
      VkShaderModule module_vert = VK_NULL_HANDLE;
      { ZoneScopedN("module_vert");
        VkShaderModuleCreateInfo info = {
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = embedded_example_rs_vert_len,
          .pCode = (const uint32_t*) embedded_example_rs_vert,
        };
        auto result = vkCreateShaderModule(
          vulkan->core.device,
          &info,
          vulkan->core.allocator,
          &module_vert
        );
        assert(result == VK_SUCCESS);
      }
      VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = module_vert,
          .pName = "main",
        },
        {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = module_frag,
          .pName = "main",
        },
      };
      VkVertexInputBindingDescription binding_descriptions[] = {
        {
          .binding = 0,
          .stride = sizeof(mesh::VertexT05),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
      };
      VkVertexInputAttributeDescription attribute_descriptions[] = {
        {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = offsetof(mesh::VertexT05, position),
        },
        {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = offsetof(mesh::VertexT05, normal),
        }
      };
      VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = sizeof(binding_descriptions) / sizeof(*binding_descriptions),
        .pVertexBindingDescriptions = binding_descriptions,
        .vertexAttributeDescriptionCount = sizeof(attribute_descriptions) / sizeof(*attribute_descriptions),
        .pVertexAttributeDescriptions = attribute_descriptions,
      };
      VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
      };
      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(rendering->swapchain_description.image_extent.width),
        .height = float(rendering->swapchain_description.image_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
      };
      VkRect2D scissor = {
        .offset = {0, 0},
        .extent = rendering->swapchain_description.image_extent,
      };
      VkPipelineViewportStateCreateInfo viewport_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
      };
      VkPipelineRasterizationStateCreateInfo rasterizer_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
      };
      VkPipelineMultisampleStateCreateInfo multisample_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
      };
      VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
      };
      VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = (0
          | VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
        ),
      };
      VkPipelineColorBlendStateCreateInfo color_blend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
      };
      VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = nullptr,
        .layout = vulkan->example.pipeline_layout,
        .renderPass = rendering->example.render_pass,
        .subpass = 0,
      };
      {
        auto result = vkCreateGraphicsPipelines(
          vulkan->core.device,
          VK_NULL_HANDLE,
          1, &pipelineInfo,
          vulkan->core.allocator,
          &rendering->example.pipeline
        );
        assert(result == VK_SUCCESS);
      }
      vkDestroyShaderModule(vulkan->core.device, module_frag, vulkan->core.allocator);
      vkDestroyShaderModule(vulkan->core.device, module_vert, vulkan->core.allocator);
    }
    { ZoneScopedN(".image_views");
      for (auto stake : rendering->example.image_stakes) {
        VkImageView image_view;
        VkImageViewCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = stake.image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = rendering->swapchain_description.image_format,
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
          },
        };
        {
          auto result = vkCreateImageView(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &image_view
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.image_views.push_back(image_view);
      }
    }
    { ZoneScopedN(".depth_views");
      for (auto stake : rendering->example.depth_stakes) {
        VkImageView depth_view;
        VkImageViewCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = stake.image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = DEPTH_FORMAT,
          .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1,
          },
        };
        {
          auto result = vkCreateImageView(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &depth_view
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.depth_views.push_back(depth_view);
      }
    }
    { ZoneScopedN(".framebuffers");
      assert(rendering->example.image_views.size() == rendering->example.depth_views.size());
      for (size_t i = 0; i < rendering->example.image_views.size(); i++) {
        VkImageView attachments[] = {
          rendering->example.image_views[i],
          rendering->example.depth_views[i],
        };
        VkFramebuffer framebuffer;
        VkFramebufferCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = rendering->example.render_pass,
          .attachmentCount = sizeof(attachments) / sizeof(*attachments),
          .pAttachments = attachments,
          .width = rendering->swapchain_description.image_extent.width,
          .height = rendering->swapchain_description.image_extent.height,
          .layers = 1,
        };
        {
          auto result = vkCreateFramebuffer(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &framebuffer
          );
          assert(result == VK_SUCCESS);
        }
        rendering->example.framebuffers.push_back(framebuffer);
      }
    }
  }

  { ZoneScopedN(".imgui_backend");
    { ZoneScopedN(".setup_command_pool");
      VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = session->vulkan.queue_family_index,
      };
      auto result = vkCreateCommandPool(
        session->vulkan.core.device,
        &create_info,
        session->vulkan.core.allocator,
        &rendering->imgui_backend.setup_command_pool
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".setup_semaphore");
      VkSemaphoreTypeCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      };
      VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_info,
      };
      auto result = vkCreateSemaphore(
        session->vulkan.core.device,
        &create_info,
        session->vulkan.core.allocator,
        &rendering->imgui_backend.setup_semaphore
      );
      assert(result == VK_SUCCESS);
    }
    signal_imgui_setup_finished = lib::gpu_signal::create(
      &session->gpu_signal_support,
      session->vulkan.core.device,
      rendering->imgui_backend.setup_semaphore,
      1
    );
    { ZoneScopedN(".render_pass");
      VkRenderPass render_pass;
      VkAttachmentDescription color_attachment = {
        .format = rendering->swapchain_description.image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      };
      VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
      };
      VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
      };
      {
        auto result = vkCreateRenderPass(
          session->vulkan.core.device,
          &create_info,
          session->vulkan.core.allocator,
          &rendering->imgui_backend.render_pass
        );
        assert(result == VK_SUCCESS);
      }
    }
    { ZoneScopedN(".framebuffers");
      // @Note: we reuse example.image_views here.
      for (size_t i = 0; i < rendering->example.image_views.size(); i++) {
        VkImageView attachments[] = {
          rendering->example.image_views[i],
        };
        VkFramebuffer framebuffer;
        VkFramebufferCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = rendering->imgui_backend.render_pass,
          .attachmentCount = sizeof(attachments) / sizeof(*attachments),
          .pAttachments = attachments,
          .width = rendering->swapchain_description.image_extent.width,
          .height = rendering->swapchain_description.image_extent.height,
          .layers = 1,
        };
        {
          auto result = vkCreateFramebuffer(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &framebuffer
          );
          assert(result == VK_SUCCESS);
        }
        rendering->imgui_backend.framebuffers.push_back(framebuffer);
      }
    }
    { ZoneScopedN("init");
      ImGui_ImplVulkan_InitInfo info = {
        .Instance = session->vulkan.instance,
        .PhysicalDevice = session->vulkan.physical_device,
        .Device = session->vulkan.core.device,
        .QueueFamily = session->vulkan.queue_family_index,
        .Queue = session->vulkan.queue_work,
        .DescriptorPool = session->vulkan.common_descriptor_pool,
        .Subpass = 0,
        .MinImageCount = rendering->swapchain_description.image_count,
        .ImageCount = rendering->swapchain_description.image_count,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = session->vulkan.core.allocator,
      };
      ImGui_ImplVulkan_Init(&info, rendering->imgui_backend.render_pass);
    }
    { ZoneScopedN("fonts_texture");
      auto alloc_info = VkCommandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = rendering->imgui_backend.setup_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };
      vkAllocateCommandBuffers(
        session->vulkan.core.device,
        &alloc_info,
        &cmd_imgui_setup
      );
      { // begin
        VkCommandBufferBeginInfo begin_info = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        auto result = vkBeginCommandBuffer(cmd_imgui_setup, &begin_info);
        assert(result == VK_SUCCESS);
      }
      ImGui_ImplVulkan_CreateFontsTexture(cmd_imgui_setup);
      { // end
        auto result = vkEndCommandBuffer(cmd_imgui_setup);
        assert(result == VK_SUCCESS);
      }
    }
  }
  auto rendering_yarn_end = lib::task::create_yarn_signal();
  auto task_frame = task::create(
    rendering_frame,
    rendering_yarn_end,
    session.ptr,
    rendering,
    &session->glfw,
    &rendering->presentation_failure_state,
    &rendering->latest_frame,
    &rendering->swapchain_description,
    &rendering->inflight_gpu
  );
  auto task_cleanup = task::create(defer, task::create(
    rendering_cleanup,
    session_iteration_yarn_end.ptr,
    session.ptr,
    rendering
  ));
  auto task_imgui_setup_cleanup = task::create(defer, task::create(
    rendering_imgui_setup_cleanup,
    &session->vulkan.core,
    &rendering->imgui_backend
  ));
  task::inject(ctx->runner, {
    task_frame,
    task_imgui_setup_cleanup,
    task_cleanup,
  }, {
    .new_dependencies = {
      { signal_imgui_setup_finished, task_frame },
      { signal_imgui_setup_finished, task_imgui_setup_cleanup },
      { rendering_yarn_end, task_cleanup }
    },
    .changed_parents = { { .ptr = rendering, .children = {
      &rendering->presentation,
      &rendering->presentation_failure_state,
      &rendering->swapchain_description,
      &rendering->inflight_gpu,
      &rendering->imgui_backend,
      &rendering->latest_frame,
      &rendering->command_pools,
      &rendering->example_finished_semaphore,
      &rendering->imgui_finished_semaphore,
      &rendering->frame_rendered_semaphore,
      &rendering->multi_alloc,
      &rendering->example,
    } } },
  });
  { ZoneScopedN("submit_imgui_setup");
    // this needs to happen after `inject`,
    // so that the signal is still valid here.

    // check sanity
    assert(signal_imgui_setup_finished != nullptr);
    assert(cmd_imgui_setup != VK_NULL_HANDLE);

    uint64_t one = 1;
    auto timeline_info = VkTimelineSemaphoreSubmitInfo {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .signalSemaphoreValueCount = 1,
      .pSignalSemaphoreValues = &one,
    };
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_imgui_setup,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &rendering->imgui_backend.setup_semaphore,
    };
    auto result = vkQueueSubmit(
      session->vulkan.queue_work,
      1, &submit_info,
      VK_NULL_HANDLE
    );
    assert(result == VK_SUCCESS);
  }
}

void session_iteration(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<task::Task> session_yarn_end,
  usage::Some<SessionData> data
) {
  ZoneScoped;
  bool should_stop = glfwWindowShouldClose(data->glfw.window);
  if (should_stop) {
    task::signal(ctx->runner, session_yarn_end.ptr);
    return;
  } else {
    glfwWaitEvents();
    auto session_iteration_yarn_end = task::create_yarn_signal();
    auto task_try_rendering = task::create(
      session_iteration_try_rendering,
      session_iteration_yarn_end,
      data.ptr
    );
    auto task_repeat = task::create(defer, task::create(
      session_iteration,
      session_yarn_end.ptr,
      data.ptr
    ));
    task::inject(ctx->runner, {
      task_try_rendering,
      task_repeat
    }, {
      .new_dependencies = { { session_iteration_yarn_end, task_repeat } },
    });
  }
}

void session_cleanup(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::Full<SessionData> session
) {
  ZoneScoped;
  { ZoneScopedN(".gpu_signal_support");
    lib::gpu_signal::deinit_support(
      &session->gpu_signal_support,
      session->vulkan.core.device,
      session->vulkan.core.allocator
    );
  }
  { ZoneScopedN(".vulkan");
    auto it = &session->vulkan;
    { ZoneScopedN(".example");
      vkDestroyDescriptorSetLayout(
        it->core.device,
        it->example.descriptor_set_layout,
        it->core.allocator
      );
      vkDestroyPipelineLayout(
        it->core.device,
        it->example.pipeline_layout,
        it->core.allocator
      );
    }
    { ZoneScopedN(".multi_alloc");
      lib::gfx::multi_alloc::deinit(&it->multi_alloc, it->core.device, it->core.allocator);
    }
    { ZoneScopedN(".core.tracy_context");
      TracyVkDestroy(it->core.tracy_context);
    }
    vkDestroyCommandPool(
      it->core.device,
      it->tracy_setup_command_pool,
      it->core.allocator
    );
    vkDestroyDescriptorPool(
      it->core.device,
      it->common_descriptor_pool,
      it->core.allocator
    );
    auto _vkDestroyDebugUtilsMessengerEXT = 
      (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        it->instance,
        "vkDestroyDebugUtilsMessengerEXT"
      );
    _vkDestroyDebugUtilsMessengerEXT(
      it->instance,
      it->debug_messenger,
      it->core.allocator
    );
    vkDestroyDevice(it->core.device, it->core.allocator);
    vkDestroySurfaceKHR(it->instance, it->window_surface, it->core.allocator);
    vkDestroyInstance(it->instance, it->core.allocator);
    assert(it->core.allocator == nullptr); // move along, nothing to destroy
  }
  { ZoneScopedN(".imgui_context");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  } 
  { ZoneScopedN(".glfw");
    glfwDestroyWindow(session->glfw.window);
    glfwTerminate();
  }
  delete session.ptr;
  ctx->changed_parents = {
    { .ptr = session.ptr, .children = {} },
    { .ptr = &session->vulkan, .children = {} },
  };
}

void session(
  task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  usage::None<size_t> worker_count
) {
  ZoneScoped;
  auto session = new SessionData;
  auto yarn_end = task::create_yarn_signal();
  mesh::T05 the_mesh;
  { ZoneScopedN("the_mesh_load");
    the_mesh = mesh::read_t05_file("assets/mesh.t05");
  }
  { ZoneScopedN("glfw");
    auto it = &session->glfw;
    {
      bool success = glfwInit();
      assert(success == GLFW_TRUE);
    }
    glfwSetErrorCallback([](int error_code, const char* description) {
      LOG("GLFW error, code: {}, description: {}", error_code, description);
    });
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // for Vulkan
    GLFWmonitor *monitor = nullptr;
    int width = DEFAULT_WINDOW_WIDTH;
    int height = DEFAULT_WINDOW_HEIGHT;

    it->window = glfwCreateWindow(width, height, "WorkingTitle", monitor, nullptr);
    it->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    it->last_window_position = glm::ivec2(0, 0);
    it->last_window_size = glm::ivec2(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    assert(it->window != nullptr);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(it->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetKeyCallback(it->window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
      auto ptr = (GlfwUserData *) glfwGetWindowUserPointer(window);
      if (ptr == nullptr) {
        return;
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_GRAVE_ACCENT) {
        ptr->state->show_imgui = !ptr->state->show_imgui;
      }
      if (false
        || (true
          && action == GLFW_PRESS
          && key == GLFW_KEY_F11
        )
        || (true
          && action == GLFW_PRESS
          && key == GLFW_KEY_ENTER
          && (false
            || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS
          )
        )
      ) {
        ptr->state->is_fullscreen = !ptr->state->is_fullscreen;
      }
    });
  }
  { ZoneScopedN(".imgui_context");
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(session->glfw.window, true);
    session->imgui_context = {}; // for clarity. maybe in future some data will be populated here
  }
  { ZoneScopedN(".vulkan");
    auto it = &session->vulkan;

    // don't use the allocator callbacks
    const VkAllocationCallbacks *allocator = nullptr;

    { ZoneScopedN(".instance");
      uint32_t glfw_extension_count = 0;
      auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
      std::vector<const char *> extensions(
        glfw_extensions,
        glfw_extensions + glfw_extension_count
      );
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "WorkingTitle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
      };
      std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation"
      };
      const auto instance_create_info = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = (uint32_t) layers.size(),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = (uint32_t) extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
      };
      {
        auto result = vkCreateInstance(&instance_create_info, allocator, &it->instance);
        assert(result == VK_SUCCESS);
      }
    }
    { ZoneScopedN(".debug_messenger");
      auto _vkCreateDebugUtilsMessengerEXT = 
        (PFN_vkCreateDebugUtilsMessengerEXT)
          vkGetInstanceProcAddr(it->instance, "vkCreateDebugUtilsMessengerEXT");
      VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = (0
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        ),
        .messageType = (0
          | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ),
        .pfnUserCallback = vulkan_debug_callback,
      };
      auto result = _vkCreateDebugUtilsMessengerEXT(
        it->instance,
        &create_info,
        allocator,
        &it->debug_messenger
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".window_surface");
      auto result = glfwCreateWindowSurface(
        it->instance,
        session->glfw.window,
        allocator,
        &it->window_surface
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".physical_device");
      // find the first discrete GPU physical device
      // in future, this should be more nuanced.
      {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(it->instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(it->instance, &device_count, devices.data());
        for (const auto& device : devices) {
          VkPhysicalDeviceProperties properties;
          vkGetPhysicalDeviceProperties(device, &properties);
          if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            it->physical_device = device;
            break;
          }
        }
        assert(it->physical_device != VK_NULL_HANDLE);
      }
    }

    // find a queue family that supports everything we need:
    // graphics, compute, transfer, present.
    // this probably works on most (all?) modern desktop GPUs.
    uint32_t queue_family_index = (uint32_t) -1;
    { ZoneScopedN("family_indices");
      uint32_t queue_family_count = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(
        it->physical_device,
        &queue_family_count,
        nullptr
      );
      std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
      vkGetPhysicalDeviceQueueFamilyProperties(
        it->physical_device,
        &queue_family_count,
        queue_families.data()
      );
      {
        int i = 0;
        for (const auto& queue_family : queue_families) {
          VkBool32 has_present_support = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(
            it->physical_device,
            i,
            it->window_surface,
            &has_present_support
          );
          if (has_present_support
            && (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            && (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
          ) {
            queue_family_index = i;
            break;
          }
        }
      }
      assert(queue_family_index != (uint32_t) -1);
    }
    { ZoneScopedN(".core");
      float queue_priorities[] = { 1.0f, 1.0f };
      VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
        .queueCount = 2,
        .pQueuePriorities = queue_priorities,
      };
      const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        /*
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, // for RAY_TRACING_PIPELINE
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // for ACCELERATION_STRUCTURE
        */
        VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, // for Tracy
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, // for shader printf
      };
      VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .timelineSemaphore = VK_TRUE,
      };
      VkPhysicalDeviceBufferDeviceAddressFeaturesEXT buffer_device_address_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
        .pNext = &timeline_semaphore_features,
        .bufferDeviceAddress = VK_TRUE,
      };
      VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &buffer_device_address_features,
        .accelerationStructure = VK_TRUE,
      };
      VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &acceleration_structure_features,
        .rayTracingPipeline = VK_TRUE,
      };
      VkPhysicalDeviceFeatures device_features = {
        .samplerAnisotropy = VK_TRUE,
      };
      const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &ray_tracing_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = (uint32_t) device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &device_features,
      };
      {
        auto result = vkCreateDevice(
          it->physical_device,
          &device_create_info,
          allocator,
          &it->core.device
        );
        assert(result == VK_SUCCESS);
      }
      it->core.allocator = allocator;
    }
    { ZoneScopedN("queues");
      vkGetDeviceQueue(it->core.device, queue_family_index, 0, &it->queue_present);
      vkGetDeviceQueue(it->core.device, queue_family_index, 1, &it->queue_work);
      it->queue_family_index = queue_family_index;
      assert(it->queue_present != VK_NULL_HANDLE);
      assert(it->queue_work != VK_NULL_HANDLE);
    }
    { ZoneScopedN(".properties");
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(it->physical_device, &it->properties.basic);

      VkPhysicalDeviceMemoryProperties memory_properties;
      vkGetPhysicalDeviceMemoryProperties(it->physical_device, &it->properties.memory);

      it->properties.ray_tracing = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
      };
      {
        VkPhysicalDeviceProperties2 properties2 = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
          .pNext = &it->properties.ray_tracing,
        };
        vkGetPhysicalDeviceProperties2(it->physical_device, &properties2);
      }
    }
    { ZoneScopedN(".tracy_setup_command_pool");
      VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = it->queue_family_index,
      };
      auto result = vkCreateCommandPool(
        it->core.device,
        &create_info,
        it->core.allocator,
        &it->tracy_setup_command_pool
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".common_descriptor_pool");
      // "large enough"
      const uint32_t COMMON_DESCRIPTOR_COUNT = 1024;
      const uint32_t COMMON_DESCRIPTOR_MAX_SETS = 256;

      VkDescriptorPoolSize sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, COMMON_DESCRIPTOR_COUNT },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, COMMON_DESCRIPTOR_COUNT },
      };
      VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 
        .maxSets = COMMON_DESCRIPTOR_MAX_SETS,
        .poolSizeCount = 1,
        .pPoolSizes = sizes,
      };
      auto result = vkCreateDescriptorPool(
        it->core.device,
        &create_info,
        it->core.allocator,
        &it->common_descriptor_pool
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN(".multi_alloc");
      std::vector<lib::gfx::multi_alloc::Claim> claims;
      claims.push_back({
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = the_mesh.triangle_count * 3 * sizeof(mesh::VertexT05),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        ),
        .p_stake_buffer = &it->example.vertex_stake,
      });
      lib::gfx::multi_alloc::init(
        &it->multi_alloc,
        std::move(claims),
        it->core.device,
        it->core.allocator,
        &it->properties.basic,
        &it->properties.memory
      );
    }
    { ZoneScopedN(".example");
      { ZoneScopedN(".descriptor_set_layout");
        VkDescriptorSetLayoutBinding layout_bindings[] = {
          {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
          },
          {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
          },
        };
        VkDescriptorSetLayoutCreateInfo create_info = {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .bindingCount = sizeof(layout_bindings) / sizeof(*layout_bindings),
          .pBindings = layout_bindings,
        };
        {
          auto result = vkCreateDescriptorSetLayout(
            it->core.device,
            &create_info,
            it->core.allocator,
            &it->example.descriptor_set_layout
          );
          assert(result == VK_SUCCESS);
        }
      }
      { ZoneScopedN(".descriptor_set");
        VkDescriptorSetAllocateInfo allocate_info = {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = it->common_descriptor_pool,
          .descriptorSetCount = 1,
          .pSetLayouts = &it->example.descriptor_set_layout,
        };
        {
          auto result = vkAllocateDescriptorSets(
            it->core.device,
            &allocate_info,
            &it->example.descriptor_set
          );
          assert(result == VK_SUCCESS);
        }
      }
      { ZoneScopedN(".pipeline_layout");
        VkPipelineLayoutCreateInfo info = {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .setLayoutCount = 1,
          .pSetLayouts = &it->example.descriptor_set_layout,
        };
        {
          auto result = vkCreatePipelineLayout(
            it->core.device,
            &info,
            it->core.allocator,
            &it->example.pipeline_layout
          );
          assert(result == VK_SUCCESS);
        }
      }
    }
    it->example.triangle_count = the_mesh.triangle_count;
    { ZoneScopedN("copy_vertex_buffer_data");
      void * data;
      {
        auto result = vkMapMemory(
          it->core.device,
          it->example.vertex_stake.memory,
          it->example.vertex_stake.offset,
          it->example.vertex_stake.size,
          0,
          &data
        );
        assert(result == VK_SUCCESS);
      }
      memcpy(data, the_mesh.vertices, the_mesh.triangle_count * 3 * sizeof(mesh::VertexT05));
      vkUnmapMemory(
        it->core.device,
        it->example.vertex_stake.memory
      );
    }
    { ZoneScopedN(".core.tracy_context");
      auto gpdctd = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
        vkGetInstanceProcAddr(session->vulkan.instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
      auto gct = (PFN_vkGetCalibratedTimestampsEXT)
        vkGetInstanceProcAddr(session->vulkan.instance, "vkGetCalibratedTimestampsEXT");
      assert(gpdctd != nullptr);
      assert(gct != nullptr);
      VkCommandBuffer cmd;
      {
        auto info = VkCommandBufferAllocateInfo {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = session->vulkan.tracy_setup_command_pool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
        };
        auto result = vkAllocateCommandBuffers(session->vulkan.core.device, &info, &cmd);
        assert(result == VK_SUCCESS);
      }
      session->vulkan.core.tracy_context = TracyVkContextCalibrated(
        session->vulkan.physical_device,
        session->vulkan.core.device,
        session->vulkan.queue_work,
        cmd,
        gpdctd,
        gct
      );
    }
  }
  { ZoneScopedN(".gpu_signal_support");
    session->gpu_signal_support = lib::gpu_signal::init_support(
      ctx->runner,
      session->vulkan.core.device,
      session->vulkan.core.allocator
    );
  }
  session->info.worker_count = *worker_count;
  session->state = { .debug_camera = lib::debug_camera::init() };
  session->state.debug_camera.position = glm::vec3(0.0f, -5.0f, 0.0f);
  // session is fully initialized at this point
  { ZoneScopedN("the_mesh_unload");
    mesh::deinit_t05(&the_mesh);
  }

  auto task_iteration = task::create(session_iteration, yarn_end, session);
  auto task_cleanup = task::create(defer, task::create(session_cleanup, session));
  task::inject(ctx->runner, {
    task_iteration,
    task_cleanup,
  }, {
    .new_dependencies = {
      { yarn_end, task_cleanup }
    },
    .changed_parents = {
      {
        .ptr = session,
        .children = {
          &session->glfw,
          &session->vulkan,
          &session->imgui_context,
          &session->gpu_signal_support,
          &session->info,
          &session->state,
        }
      },
      {
        .ptr = &session->vulkan,
        .children = {
          &session->vulkan.instance,
          &session->vulkan.debug_messenger,
          &session->vulkan.window_surface,
          &session->vulkan.physical_device,
          &session->vulkan.core,
          &session->vulkan.properties,
          &session->vulkan.queue_present,
          &session->vulkan.queue_work,
          &session->vulkan.queue_family_index,
          &session->vulkan.tracy_setup_command_pool,
          &session->vulkan.common_descriptor_pool,
          &session->vulkan.multi_alloc,
          &session->vulkan.example
        }
      },
    },
  });
}

const task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

const task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
  #if ENGINE_DEBUG_TASK_THREADS == 1
    | QUEUE_ACCESS_FLAGS_WORKER_THREAD
  #endif
);

int main() {
  #ifdef ENGINE_DEBUG_TASK_THREADS
    unsigned int num_threads = ENGINE_DEBUG_TASK_THREADS;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  #if ENGINE_DEBUG_TASK_THREADS == 1
    size_t worker_count = 1;
    std::vector<task::QueueAccessFlags> worker_access_flags;
  #else
    size_t worker_count = num_threads - 1;
    std::vector<task::QueueAccessFlags> worker_access_flags(worker_count, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
  #endif
  auto runner = task::create_runner(QUEUE_COUNT);
  task::inject(runner, {
    task::create(session, &worker_count),
  });
  task::run(runner, std::move(worker_access_flags), QUEUE_ACCESS_FLAGS_MAIN_THREAD);
  task::discard_runner(runner);
  #ifdef TRACY_NO_EXIT
    printf("Waiting for profiler...\n");
    fflush(stdout);
  #endif
  return 0;
}

#ifdef WINDOWS
  int WinMain() {
    return main();
  }
#endif