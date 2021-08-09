#include <imgui.h>
#include <stb_image.h>
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <src/embedded.hxx>
#include <src/engine/texture.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <src/engine/loading/group.hxx>
#include <src/engine/misc.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "defer.hxx"
#include "session_cleanup.hxx"
#include "session_iteration.hxx"
#include "session_setup_cleanup.hxx"
#include "session.hxx"

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

const int DEFAULT_WINDOW_WIDTH = 1280;
const int DEFAULT_WINDOW_HEIGHT = 720;

glm::vec2 fullscreen_quad_data[] = { // not a quad now!
  { -1.0f, -1.0f },
  { -1.0f, +3.0f },
  { +3.0f, -1.0f },
};

void init_vulkan(
  SessionData::Vulkan *it,
  SessionData::GLFW *glfw
) {
  ZoneScoped;
  auto core = &it->core;

  /*
  SessionSetupData *session_setup_data;
  { ZoneScopedN("session_setup_data");
    session_setup_data = new SessionSetupData {};
  }
  */

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
        // | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
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
      glfw->window,
      allocator,
      &it->window_surface
    );
    assert(result == VK_SUCCESS);
  }

  { ZoneScopedN(".physical_device");
    // find the first discrete GPU physical device
    // in future, this should be more nuanced.
    {
      VkPhysicalDevice physical_device = nullptr;
      uint32_t device_count = 0;
      vkEnumeratePhysicalDevices(it->instance, &device_count, nullptr);
      std::vector<VkPhysicalDevice> devices(device_count);
      vkEnumeratePhysicalDevices(it->instance, &device_count, devices.data());
      for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          physical_device = device;
          break;
        }
      }
      assert(physical_device != VK_NULL_HANDLE);
      it->physical_device = physical_device;
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
    it->core.queue_family_index = queue_family_index;
    assert(it->queue_present != VK_NULL_HANDLE);
    assert(it->queue_work != VK_NULL_HANDLE);
  }

  { ZoneScopedN(".core.properties");
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(it->physical_device, &it->core.properties.basic);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(it->physical_device, &it->core.properties.memory);

    it->core.properties.ray_tracing = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };
    {
      VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &it->core.properties.ray_tracing,
      };
      vkGetPhysicalDeviceProperties2(it->physical_device, &properties2);
    }
  }

  { ZoneScopedN(".tracy_setup_command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = it->core.queue_family_index,
    };
    auto result = vkCreateCommandPool(
      it->core.device,
      &create_info,
      it->core.allocator,
      &it->tracy_setup_command_pool
    );
    assert(result == VK_SUCCESS);
  }

  { ZoneScopedN(".multi_alloc");
    std::vector<lib::gfx::multi_alloc::Claim> claims;
    claims.push_back({
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = sizeof(fullscreen_quad_data),
          .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
      },
      .memory_property_flags = VkMemoryPropertyFlagBits(0
        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      .p_stake_buffer = &it->fullscreen_quad.vertex_stake,
    });

    lib::gfx::multi_alloc::init(
      &it->multi_alloc,
      std::move(claims),
      it->core.device,
      it->core.allocator,
      &it->core.properties.basic,
      &it->core.properties.memory
    );
  }

  // for both pipelines
  VkShaderModule module_frag = VK_NULL_HANDLE;
  VkShaderModule module_vert = VK_NULL_HANDLE;

  { ZoneScopedN("module_vert");
    VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = embedded_gpass_vert_len,
      .pCode = (const uint32_t*) embedded_gpass_vert,
    };
    auto result = vkCreateShaderModule(
      it->core.device,
      &info,
      it->core.allocator,
      &module_vert
    );
    assert(result == VK_SUCCESS);
  }

  { ZoneScopedN("module_frag");
    VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = embedded_gpass_frag_len,
      .pCode = (const uint32_t*) embedded_gpass_frag,
    };
    auto result = vkCreateShaderModule(
      it->core.device,
      &info,
      it->core.allocator,
      &module_frag
    );
    assert(result == VK_SUCCESS);
  }

  init_session_gpass(
    &it->gpass,
    &it->core,
    module_vert,
    module_frag
  );

  init_session_prepass(
    &it->prepass,
    &it->gpass,
    &it->core,
    module_vert
  );

  init_session_lpass(
    &it->lpass,
    &it->core
  );

  init_session_finalpass(
    &it->finalpass,
    &it->core
  );

  vkDestroyShaderModule(
    it->core.device,
    module_frag,
    it->core.allocator
  );
  vkDestroyShaderModule(
    it->core.device,
    module_vert,
    it->core.allocator
  );

  { ZoneScopedN(".fullscreen_quad");
    it->fullscreen_quad.triangle_count = sizeof(fullscreen_quad_data) / sizeof(glm::vec2);
    void * data;
    {
      auto result = vkMapMemory(
        it->core.device,
        it->fullscreen_quad.vertex_stake.memory,
        it->fullscreen_quad.vertex_stake.offset,
        it->fullscreen_quad.vertex_stake.size,
        0,
        &data
      );
      assert(result == VK_SUCCESS);
    }
    assert(it->fullscreen_quad.vertex_stake.size == sizeof(fullscreen_quad_data));
    memcpy(data, fullscreen_quad_data, sizeof(fullscreen_quad_data));
    vkUnmapMemory(
      it->core.device,
      it->fullscreen_quad.vertex_stake.memory
    );
  }

  { ZoneScopedN(".core.tracy_context");
    auto gpdctd = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
      vkGetInstanceProcAddr(it->instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
    auto gct = (PFN_vkGetCalibratedTimestampsEXT)
      vkGetInstanceProcAddr(it->instance, "vkGetCalibratedTimestampsEXT");
    assert(gpdctd != nullptr);
    assert(gct != nullptr);
    VkCommandBuffer cmd;
    {
      auto info = VkCommandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = it->tracy_setup_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };
      auto result = vkAllocateCommandBuffers(it->core.device, &info, &cmd);
      assert(result == VK_SUCCESS);
    }
    #ifdef TRACY_ENABLE
      it->core.tracy_context = TracyVkContextCalibrated(
        it->physical_device,
        it->core.device,
        it->queue_work,
        cmd,
        gpdctd,
        gct
      );
    #endif
  }

  it->ready = true;

  /*
  VkSemaphore semaphore_setup_finished;
  { ZoneScopedN("semaphore_setup_finished");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    {
      auto result = vkCreateSemaphore(
        it->core.device,
        &create_info,
        it->core.allocator,
        &semaphore_setup_finished
      );
      assert(result == VK_SUCCESS);
    }
  }

  VkCommandPool setup_command_pool;
  { ZoneScopedN("setup_command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = core->queue_family_index,
    };
    auto result = vkCreateCommandPool(
      core->device,
      &create_info,
      core->allocator,
      &setup_command_pool
    );
    assert(result == VK_SUCCESS);
  }

  session_setup_data->semaphore_finished = semaphore_setup_finished;
  session_setup_data->command_pool = setup_command_pool;

  return session_setup_data;
  */
}

TASK_DECL {
  ZoneScoped;
  auto session = new SessionData {};
  lib::lifetime::init(&session->lifetime);

  { ZoneScopedN(".glfw");
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
      auto ptr = (engine::misc::GlfwUserData *) glfwGetWindowUserPointer(window);
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
      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_G
        && (false
          || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
          || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
        )
      ) {
        ptr->state->show_imgui_window_groups = !ptr->state->show_imgui_window_groups;
      }
      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_M
        && (false
          || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
          || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
        )
      ) {
        ptr->state->show_imgui_window_meshes = !ptr->state->show_imgui_window_meshes;
      }
      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_D
        && (false
          || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
          || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
        )
      ) {
        ptr->state->show_imgui_window_demo = !ptr->state->show_imgui_window_demo;
      }
    });
    it->ready = true;
  }
  auto glfw = &session->glfw;

  { ZoneScopedN(".imgui_context");
    auto it = &session->imgui_context;
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(glfw->window, true);
    it->ready = true;
  }

  auto vulkan = &session->vulkan;
  init_vulkan(vulkan, glfw);

  { ZoneScopedN(".gpu_signal_support");
    auto it = &session->gpu_signal_support;
    *it = lib::gpu_signal::init_support(
      ctx->runner,
      vulkan->core.device,
      vulkan->core.allocator
    );
  }

  session->info = {
    .worker_count = *worker_count,
  };

  session->state = {
    .debug_camera = lib::debug_camera::init(),
  };

  /*
  task::Task* signal_setup_finished = lib::gpu_signal::create(
    &session->gpu_signal_support,
    vulkan->core.device,
    session_setup_data->semaphore_finished,
    1
  );
  */

  // when the structs change, recheck that .changed_parents are still valid.
  #ifndef NDEBUG
  {
    const auto size = sizeof(SessionData);
    static_assert(size == 2784);
  }
  {
    const auto size = sizeof(SessionData::Vulkan);
    static_assert(size == 1920);
  }
  #endif

  auto task_iteration = task::create(
    session_iteration,
    session
  );

  /*
  auto task_setup_cleanup = task::create(
    session_setup_cleanup,
    session_setup_data,
    &session->vulkan.core
  );
  */

  auto task_cleanup = defer(
    lib::task::create(
      session_cleanup,
      session
    )
  );

  ctx->changed_parents.insert(ctx->changed_parents.end(), {
    {
      .ptr = session,
      .children = {
        &session->glfw,
        &session->guid_counter,
        &session->inflight_gpu,
        &session->lifetime,
        &session->scene,
        &session->meta_meshes,
        &session->meta_textures,
        &session->vulkan,
        &session->imgui_context,
        &session->gpu_signal_support,
        &session->info,
        &session->state,
      },
    },
    {
      .ptr = &session->vulkan,
      .children = {
        &session->vulkan.instance,
        &session->vulkan.debug_messenger,
        &session->vulkan.window_surface,
        &session->vulkan.physical_device,
        &session->vulkan.core,
        &session->vulkan.queue_present,
        &session->vulkan.queue_work,
        &session->vulkan.core.queue_family_index,
        &session->vulkan.tracy_setup_command_pool,
        &session->vulkan.multi_alloc,
        &session->vulkan.fullscreen_quad,
        &session->vulkan.gpass,
        &session->vulkan.lpass,
        &session->vulkan.finalpass,
        &session->vulkan.meshes,
        &session->vulkan.textures,
      },
    },
  });

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_iteration,
    task_cleanup.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    /*
    { signal_setup_finished, task_iteration },
    { signal_setup_finished, task_setup_cleanup },
    */
    { session->lifetime.yarn, task_cleanup.first }
  });
}
