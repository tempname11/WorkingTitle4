#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_set>
#include <src/lib/task.hxx>
#include <src/global.hxx>

namespace task {
  using namespace lib::task;
}

const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;

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

struct RenderingData {
  task::AccessMarker _;
  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapchain_images;
  struct SwapchainDescription {
    task::AccessMarker _;
    size_t swapchain_image_count;
    VkExtent2D swapchain_image_extent;
    VkFormat swapchain_image_format;
  } swapchain_description;
};

struct SessionData {
  task::AccessMarker _;
  GLFWwindow *window;
  struct Vulkan {
    task::AccessMarker _;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR window_surface;
    VkPhysicalDevice physical_device;
    struct Core {
      task::AccessMarker _;
      VkDevice device;
      const VkAllocationCallbacks *allocator;
    } core;
    struct CoreProperties {
      task::AccessMarker _;
      VkPhysicalDeviceProperties basic;
      VkPhysicalDeviceMemoryProperties memory;
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing;
    } properties;
    VkQueue queue_present;
    VkQueue queue_gct; // graphics, compute, transfer
  } vulkan;
};

void defer(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_HIGH_PRIORITY>,
  task::Exclusive<task::Task> task
) {
  task::inject(ctx->runner, { task.ptr });
}

void session_iteration_cleanup(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_LOW_PRIORITY>,
  task::Exclusive<task::Task> session_iteration_stop_signal,
  task::Shared<SessionData> session,
  task::Exclusive<RenderingData> data
) {
  ZoneScoped;
  auto vulkan = &session->vulkan;
  vkDestroySwapchainKHR(
    vulkan->core.device,
    data->swapchain,
    vulkan->core.allocator
  );
  delete data.ptr;
  task::signal(ctx->runner, session_iteration_stop_signal.ptr);
}

void rendering_frame(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  task::Exclusive<task::Task> rendering_stop_signal,
  task::Shared<SessionData> session,
  task::Exclusive<RenderingData> data
) {
  ZoneScoped;
  FrameMark;
  bool should_stop = glfwWindowShouldClose(session->window);
  if (should_stop) {
    lib::task::signal(ctx->runner, rendering_stop_signal.ptr);
  } else {
    glfwPollEvents(); // TODO move this
    task::inject(ctx->runner, {
      task::create(rendering_frame, rendering_stop_signal.ptr, session.ptr, data.ptr),
    });
  }
}

void session_iteration_try_rendering(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_HIGH_PRIORITY>,
  task::Exclusive<task::Task> session_iteration_stop_signal,
  task::Exclusive<SessionData> data
) {
  ZoneScoped;
  auto vulkan = &data->vulkan;
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
    lib::task::signal(ctx->runner, session_iteration_stop_signal.ptr);
    return;
  }

  auto rendering = new RenderingData;
  { ZoneScopedN("swapchain");
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
        &rendering->swapchain
      );
      assert(result == VK_SUCCESS);
    }
  }


  auto rendering_stop_signal = lib::task::create_signal();
  auto task_frame = task::create(
    rendering_frame,
    rendering_stop_signal,
    data.ptr,
    rendering
  );
  auto task_cleanup = task::create(defer, task::create(
    session_iteration_cleanup,
    session_iteration_stop_signal.ptr,
    data.ptr,
    rendering
  ));
  task::inject(ctx->runner, {
    task_frame,
    task_cleanup,
  }, {
    { rendering_stop_signal, task_cleanup },
  });
}

void session_iteration(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  task::Exclusive<task::Task> session_stop_signal,
  task::Exclusive<SessionData> data
) {
  ZoneScoped;
  bool should_stop = glfwWindowShouldClose(data->window);
  if (should_stop) {
    task::signal(ctx->runner, session_stop_signal.ptr);
    return;
  } else {
    glfwWaitEvents();
    auto session_iteration_stop_signal = task::create_signal();
    auto task_try_rendering = task::create(
      session_iteration_try_rendering,
      session_iteration_stop_signal,
      data.ptr
    );
    auto task_repeat = task::create(defer, task::create(
      session_iteration,
      session_stop_signal.ptr,
      data.ptr
    ));
    task::inject(ctx->runner, {
      task_try_rendering,
      task_repeat
    }, {
      { session_iteration_stop_signal, task_repeat }
    });
  }
}

void session_cleanup(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>,
  task::Exclusive<SessionData> session
) {
  ZoneScoped;
  { ZoneScopedN("vulkan");
    auto it = &session->vulkan;
    auto allocator = it->core.allocator;
    auto _vkDestroyDebugUtilsMessengerEXT = 
      (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        it->instance,
        "vkDestroyDebugUtilsMessengerEXT"
      );
    _vkDestroyDebugUtilsMessengerEXT(it->instance, it->debug_messenger, allocator);
    vkDestroyDevice(it->core.device, allocator);
    assert(it->core.allocator == nullptr);
    vkDestroySurfaceKHR(it->instance, it->window_surface, allocator);
    vkDestroyInstance(it->instance, allocator);
  }
  { ZoneScopedN("glfw");
    glfwDestroyWindow(session->window);
    glfwTerminate();
  }
  delete session.ptr;
}

void session(
  task::Context *ctx,
  task::QueueMarker<QUEUE_INDEX_MAIN_THREAD_ONLY>
) {
  ZoneScoped;
  auto session = new SessionData;
  auto stop_signal = task::create_signal();
  { ZoneScopedN("glfw");
    auto it = &session->window;
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
    #if 0
      // duplicates code in process_settings
      monitor = glfwGetPrimaryMonitor();
      auto mode = glfwGetVideoMode(monitor);
      // hints for borderless fullscreen
      glfwWindowHint(GLFW_RED_BITS, mode->redBits);
      glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
      glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
      glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
      width = mode->width;
      height = mode->height;
    #endif
    *it = glfwCreateWindow(width, height, "WorkingTitle", monitor, nullptr);
    assert(*it != nullptr);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(*it, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  }
  { ZoneScopedN("vulkan");
    auto it = &session->vulkan;

    // don't use the allocator callbacks
    const VkAllocationCallbacks *allocator = nullptr;

    { ZoneScopedN("instance");
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
    { ZoneScopedN("debug_messenger");
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
    { ZoneScopedN("window_surface");
      auto result = glfwCreateWindowSurface(
        it->instance,
        session->window,
        allocator,
        &it->window_surface
      );
      assert(result == VK_SUCCESS);
    }
    { ZoneScopedN("physical_device");
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

    uint32_t queue_present_family_index = (uint32_t) -1;
    // find a queue family that supports everything else we need:
    // Graphics, Compute, Transfer
    // this probably works on most (all?) modern desktop GPUs.
    uint32_t queue_gct_family_index = (uint32_t) -1;
    { ZoneScopedN("family-indices");
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
          if (has_present_support) {
            queue_present_family_index = i;
          }
          if (true
            && (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            && (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
          ) {
            queue_gct_family_index = i;
          }
          if (true
            && queue_present_family_index != (uint32_t) -1
            && queue_gct_family_index != (uint32_t) -1
          ) {
            break;
          }
        }
      }
      assert(queue_present_family_index != (uint32_t) -1);
      assert(queue_gct_family_index != (uint32_t) -1);
    }
    { ZoneScopedN("core");
      std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
      if (queue_gct_family_index == queue_present_family_index) {
        float queue_priorities[2] = { 1.0f, 1.0f };
        queue_create_infos.push_back(VkDeviceQueueCreateInfo {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queue_gct_family_index,
          .queueCount = 2,
          .pQueuePriorities = queue_priorities,
        });
      } else {
        float queue_priority = 1.0f;
        queue_create_infos.push_back(VkDeviceQueueCreateInfo {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queue_gct_family_index,
          .queueCount = 1,
          .pQueuePriorities = &queue_priority,
        });
        queue_create_infos.push_back(VkDeviceQueueCreateInfo {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queue_present_family_index,
          .queueCount = 1,
          .pQueuePriorities = &queue_priority,
        });
      }
      const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, // for RAY_TRACING_PIPELINE
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // for ACCELERATION_STRUCTURE
        VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
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
        .queueCreateInfoCount = (uint32_t) queue_create_infos.size(),
        .pQueueCreateInfos = queue_create_infos.data(),
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
      if (queue_gct_family_index == queue_present_family_index) {
        vkGetDeviceQueue(it->core.device, queue_present_family_index, 0, &it->queue_present);
        vkGetDeviceQueue(it->core.device, queue_gct_family_index, 1, &it->queue_gct);
      } else {
        vkGetDeviceQueue(it->core.device, queue_present_family_index, 0, &it->queue_present);
        vkGetDeviceQueue(it->core.device, queue_gct_family_index, 0, &it->queue_gct);
      }
      assert(it->queue_present != VK_NULL_HANDLE);
      assert(it->queue_gct != VK_NULL_HANDLE);
    }
    { ZoneScopedN("properties");
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
  }
  // session is fully initialized at this point

  auto task_iteration = task::create(session_iteration, stop_signal, session);
  auto task_cleanup = task::create(defer, task::create(session_cleanup, session));
  task::inject(ctx->runner, {
    task_iteration,
    task_cleanup,
  }, {
    { stop_signal, task_cleanup }
  });
}

const task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
);

const task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

int main() {
  auto runner = task::create_runner(QUEUE_COUNT);
  task::inject(runner, {
    task::create(session),
  });
  #ifndef NDEBUG
    auto num_threads = 4u;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  std::vector worker_access_flags(num_threads - 1, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
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