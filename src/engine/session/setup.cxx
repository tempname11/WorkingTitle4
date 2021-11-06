#include <imgui.h>
#include <stb_image.h>
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <src/embedded.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/lib/defer.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <src/engine/datum/probe_workset.hxx>
#include <src/engine/datum/probe_confidence.hxx>
#include <src/engine/datum/probe_offsets.hxx>
#include <src/engine/step/probe_appoint.hxx>
#include <src/engine/step/probe_measure.hxx>
#include <src/engine/step/probe_collect.hxx>
#include <src/engine/step/indirect_light.hxx>
#include <src/engine/system/grup/group.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/misc.hxx>
#include "setup_cleanup.hxx"
#include "iteration.hxx"
#include "public.hxx"

namespace engine::session {

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

const int DEFAULT_WINDOW_WIDTH = 1920;
const int DEFAULT_WINDOW_HEIGHT = 1080;

glm::vec2 fullscreen_quad_data[] = { // not a quad now!
  { -1.0f, -1.0f },
  { -1.0f, +3.0f },
  { +3.0f, -1.0f },
};

void init_vulkan(
  engine::session::Vulkan *it,
  engine::session::Data::GLFW *glfw
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
      "VK_LAYER_KHRONOS_validation" // @Cleanup: use env var
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
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // for ACCELERATION_STRUCTURE
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_QUERY_EXTENSION_NAME,
      // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, // for Tracy
      VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, // for shader printf
    };
    VkPhysicalDevice16BitStorageFeatures sixteen_bit_storage_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
      .storageBuffer16BitAccess = VK_TRUE,
    };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
      .pNext = &sixteen_bit_storage_features,
      .accelerationStructure = VK_TRUE,
    };
    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
      .pNext = &acceleration_structure_features,
      .rayQuery = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features vulkan_1_2_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &ray_query_features,
      //.storageBuffer8BitAccess = VK_TRUE,
      //.shaderInt8 = VK_TRUE,
      .descriptorIndexing = VK_TRUE,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      //.descriptorBindingPartiallyBound = VK_TRUE,
      .descriptorBindingVariableDescriptorCount = VK_TRUE,
      .runtimeDescriptorArray = VK_TRUE,
      .timelineSemaphore = VK_TRUE,
      .bufferDeviceAddress = VK_TRUE,
    };
    /*
    VkPhysicalDeviceRayQueryFeaturesKHR ray_tracing_pipeline_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
      .pNext = &acceleration_structure_features,
      .rayTracingPipeline = VK_TRUE,
    };
    */
    VkPhysicalDeviceFeatures device_features = {
      .samplerAnisotropy = VK_TRUE,
      .shaderStorageImageWriteWithoutFormat = VK_TRUE,
      .shaderInt16 = VK_TRUE,
    };
    const VkDeviceCreateInfo device_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &vulkan_1_2_features,
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

  { ZoneScopedN(".core.extension_pointers");
    it->core.extension_pointers.vkCreateAccelerationStructureKHR = (
      (PFN_vkCreateAccelerationStructureKHR)
      vkGetInstanceProcAddr(it->instance, "vkCreateAccelerationStructureKHR")
    );

    it->core.extension_pointers.vkDestroyAccelerationStructureKHR = (
      (PFN_vkDestroyAccelerationStructureKHR)
      vkGetInstanceProcAddr(it->instance, "vkDestroyAccelerationStructureKHR")
    );

    it->core.extension_pointers.vkGetAccelerationStructureBuildSizesKHR = (
      (PFN_vkGetAccelerationStructureBuildSizesKHR)
      vkGetInstanceProcAddr(it->instance, "vkGetAccelerationStructureBuildSizesKHR")
    );

    it->core.extension_pointers.vkCmdBuildAccelerationStructuresKHR = (
      (PFN_vkCmdBuildAccelerationStructuresKHR)
      vkGetInstanceProcAddr(it->instance, "vkCmdBuildAccelerationStructuresKHR")
    );

    it->core.extension_pointers.vkGetAccelerationStructureDeviceAddressKHR = (
      (PFN_vkGetAccelerationStructureDeviceAddressKHR)
      vkGetInstanceProcAddr(it->instance, "vkGetAccelerationStructureDeviceAddressKHR")
    );
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

  { ZoneScopedN(".allocator_host");
    lib::gfx::allocator::init(
      &it->allocator_host,
      &it->core.properties.memory,
      VkMemoryPropertyFlagBits(0
        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
      ALLOCATOR_GPU_LOCAL_REGION_SIZE,
      "session.allocator_host"
    );
  }

  { ZoneScopedN(".allocator_device");
    lib::gfx::allocator::init(
      &it->allocator_device,
      &it->core.properties.memory,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      ALLOCATOR_GPU_LOCAL_REGION_SIZE,
      "session.allocator_device"
    );
  }

  { ZoneScopedN(".uploader");
    engine::uploader::init(
      &it->uploader,
      it->core.device,
      it->core.allocator,
      &it->core.properties.memory,
      it->core.queue_family_index,
      ALLOCATOR_GPU_LOCAL_REGION_SIZE
    );
  }

  { ZoneScopedN(".blas_storage");
    engine::blas_storage::init(
      &it->blas_storage,
      &it->core,
      ALLOCATOR_GPU_LOCAL_REGION_SIZE
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

  datum::probe_workset::init_sdata(
    &it->probe_workset,
    &it->core,
    &it->allocator_device
  );

  datum::probe_confidence::init_sdata(
    &it->probe_confidence,
    &it->allocator_device,
    &it->core
  );

  datum::probe_offsets::init_sdata(
    &it->probe_offsets,
    &it->allocator_device,
    &it->core
  );

  step::probe_appoint::init_sdata(
    &it->probe_appoint,
    &it->core
  );

  step::probe_measure::init_sdata(
    &it->probe_measure,
    &it->core
  );

  step::probe_collect::init_sdata(
    &it->probe_collect,
    &it->core
  );

  step::indirect_light::init_sdata(
    &it->indirect_light,
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

  #ifdef TRACY_ENABLE
    { ZoneScopedN(".core.tracy_context");
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
        ZoneValue(uint64_t(cmd));
      }

      if (!getenv("ENGINE_ENV_TRACY_VK_UNCALIBRATED")) {
        auto gpdctd = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
          vkGetInstanceProcAddr(it->instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");

        auto gct = (PFN_vkGetCalibratedTimestampsEXT)
          vkGetInstanceProcAddr(it->instance, "vkGetCalibratedTimestampsEXT");

        assert(gpdctd != nullptr);
        assert(gct != nullptr);

        it->core.tracy_context = TracyVkContextCalibrated(
          it->physical_device,
          it->core.device,
          it->queue_work,
          cmd,
          gpdctd,
          gct
        );
      } else {
        it->core.tracy_context = TracyVkContext(
          it->physical_device,
          it->core.device,
          it->queue_work,
          cmd
        );
      }
    }
  # else
    it->core.tracy_context = nullptr;
  #endif

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


void setup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data *> out
) {
  ZoneScoped;
  auto session = new engine::session::Data {};
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
    if (out.ptr != nullptr && getenv("ENGINE_ENV_SILENT") != nullptr) {
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    GLFWmonitor *monitor = nullptr;
    int width = DEFAULT_WINDOW_WIDTH;
    int height = DEFAULT_WINDOW_HEIGHT;

    it->window = glfwCreateWindow(width, height, "WorkingTitleInstance", monitor, nullptr);
    it->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    it->last_window_position = glm::ivec2(0, 0);
    it->last_window_size = glm::ivec2(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    assert(it->window != nullptr);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(it->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetScrollCallback(it->window, [](GLFWwindow *window, double x, double y) {
      auto ptr = (engine::misc::GlfwUserData *) glfwGetWindowUserPointer(window);
      if (ptr == nullptr) {
        return;
      }
      
      if (!ptr->state->show_imgui) {
        if (y > 0.0) {
          ptr->state->movement_speed *= 2;
        }
        if (y < 0.0) {
          ptr->state->movement_speed *= 0.5;
        }
      }
    });

    glfwSetKeyCallback(it->window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
      auto ptr = (engine::misc::GlfwUserData *) glfwGetWindowUserPointer(window);
      if (ptr == nullptr) {
        return;
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_GRAVE_ACCENT) {
        ptr->state->show_imgui = !ptr->state->show_imgui;
      }

      auto is_alt_pressed = (false
        || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS
      );

      auto is_control_pressed = (false
        || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
      );

      if (false
        || (true
          && action == GLFW_PRESS
          && key == GLFW_KEY_F11
        )
        || (true
          && action == GLFW_PRESS
          && key == GLFW_KEY_ENTER
          && is_alt_pressed
        )
      ) {
        ptr->state->is_fullscreen = !ptr->state->is_fullscreen;
      }

      // Alt+R
      if (true
        && action == GLFW_PRESS
        && key == GLFW_KEY_R
        && is_alt_pressed
      ) {
        system::artline::reload_all(
          ptr->session,
          ptr->ctx
        );
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_G
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_groups = !ptr->state->show_imgui_window_groups;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_M
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_meshes = !ptr->state->show_imgui_window_meshes;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_T
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_textures = !ptr->state->show_imgui_window_textures;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_A
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_artline = !ptr->state->show_imgui_window_artline;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_D
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_demo = !ptr->state->show_imgui_window_demo;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_Q
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_gpu_memory = !ptr->state->show_imgui_window_gpu_memory;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_W
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_tools = !ptr->state->show_imgui_window_tools;
      }

      if (true
        && ptr->state->show_imgui
        && action == GLFW_PRESS
        && key == GLFW_KEY_F
        && is_control_pressed
      ) {
        ptr->state->show_imgui_window_flags = !ptr->state->show_imgui_window_flags;
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

  size_t worker_count = 0;
  for (auto f : lib::task::get_runner_info(ctx->runner)->thread_access_flags) {
    if (false
      || (f & (1 << QUEUE_INDEX_HIGH_PRIORITY))
      || (f & (1 << QUEUE_INDEX_NORMAL_PRIORITY))
      || (f & (1 << QUEUE_INDEX_LOW_PRIORITY))
    ) {
      worker_count++;
    }
  }
  session->info = {
    .worker_count = worker_count,
  };

  auto debug_camera = lib::debug_camera::init();
  session->state = {
    .ignore_glfw_events = (out.ptr != nullptr),
    .movement_speed = 8.0f,
    .debug_camera = debug_camera,
    .debug_camera_prev = debug_camera,
    .sun_irradiance = 5.0f,
    .luminance_moving_average = 0.3f,
    .taa_distance = 1.0f,
  };

  #ifdef ENGINE_DEVELOPER
    session->frame_control.enabled = (out.ptr != nullptr);
  #endif

  /*
  task::Task* signal_setup_finished = lib::gpu_signal::create(
    &session->gpu_signal_support,
    vulkan->core.device,
    session_setup_data->semaphore_finished,
    1
  );
  */

  // when the structs change, recheck that .changed_parents are still valid.
  // remove this when subresources are removed.
  #ifndef NDEBUG
  {
    const auto size = sizeof(engine::session::Data) - sizeof(engine::session::Vulkan);
    static_assert(size == 1576);
  }
  {
    const auto size = sizeof(engine::session::Vulkan);
    static_assert(size == 3256);
  }
  #endif

  auto task_iteration = lib::task::create(
    iteration,
    session
  );

  /*
  auto task_setup_cleanup = lib::task::create(
    session_setup_cleanup,
    session_setup_data,
    &session->vulkan.core
  );
  */

  auto task_cleanup = lib::defer(
    lib::task::create(
      session::cleanup,
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
        &session->grup.meta_meshes,
        &session->grup.meta_textures,
        &session->vulkan,
        &session->imgui_context,
        &session->gpu_signal_support,
        &session->info,
        &session->state,
        #ifdef ENGINE_DEVELOPER
          &session->frame_control,
        #endif
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
        &session->vulkan.uploader,
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

  if (out.ptr != nullptr) {
    *out = session;
  }
}

} // namespace
