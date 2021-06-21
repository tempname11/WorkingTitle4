#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <src/lib/gfx/mesh.hxx>
#include "task.hxx"

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
