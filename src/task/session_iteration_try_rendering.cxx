#include <backends/imgui_impl_vulkan.h>
#include <src/embedded.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/display/cleanup.hxx>
#include <src/engine/rendering/common.hxx>
#include <src/engine/rendering/image_formats.hxx>
#include <src/engine/rendering/prepass.hxx>
#include <src/engine/rendering/gpass.hxx>
#include <src/engine/rendering/lpass.hxx>
#include <src/engine/rendering/finalpass.hxx>
#include <src/engine/datum/probe_radiance.hxx>
#include <src/engine/datum/probe_irradiance.hxx>
#include <src/engine/datum/probe_attention.hxx>
#include <src/engine/step/probe_measure.hxx>
#include <src/engine/step/probe_collect.hxx>
#include <src/engine/step/indirect_light.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/frame/schedule_all.hxx>
#include "defer.hxx"
#include "rendering_imgui_setup_cleanup.hxx"
#include "session_iteration_try_rendering.hxx"

TASK_DECL {
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
  
  auto rendering = new engine::display::Data {};
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
  rendering->latest_frame.timestamp_ns = 0;
  rendering->latest_frame.elapsed_ns = 0;
  rendering->latest_frame.number = uint64_t(-1);
  rendering->latest_frame.inflight_index = uint8_t(-1);
  assert(engine::session::Data::InflightGPU::MAX_COUNT >= swapchain_image_count);

  { ZoneScopedN(".command_pools");
    rendering->command_pools = std::vector<CommandPool2>(swapchain_image_count);
    for (size_t i = 0; i < swapchain_image_count; i++) {
      rendering->command_pools[i].available.resize(session->info.worker_count);
      for (size_t j = 0; j < session->info.worker_count; j++) {
        VkCommandPool pool;
        {
          VkCommandPoolCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = session->vulkan.core.queue_family_index,
          };
          auto result = vkCreateCommandPool(
            session->vulkan.core.device,
            &create_info,
            session->vulkan.core.allocator,
            &pool
          );
          assert(result == VK_SUCCESS);
        }
        rendering->command_pools[i].available[j] = new CommandPool1 {
          .pool = pool,
        };
      }
    }
  }

  { ZoneScopedN(".descriptor_pools");
    rendering->descriptor_pools = std::vector<engine::common::SharedDescriptorPool>(
      swapchain_image_count
    );
    for (auto &pool : rendering->descriptor_pools) {
      // "large enough" :FixedDescriptorPool
      const uint32_t COMMON_DESCRIPTOR_COUNT = 4096;
      const uint32_t COMMON_DESCRIPTOR_MAX_SETS = 4096;

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
        session->vulkan.core.device,
        &create_info,
        session->vulkan.core.allocator,
        &pool.pool
      );
      assert(result == VK_SUCCESS);
    }
  }

  { ZoneScopedN(".frame_finished_semaphore");
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
      &rendering->frame_finished_semaphore
    );
    assert(result == VK_SUCCESS);
  }

  { ZoneScopedN(".graphics_finished_semaphore");
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
      &rendering->graphics_finished_semaphore
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

  auto core = &session->vulkan.core;

  lib::gfx::allocator::init(
    &rendering->allocator_dedicated,
    &core->properties.memory,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    0, // always a new region
    "display.allocator_dedicated"
  );

  lib::gfx::allocator::init(
    &rendering->allocator_shared,
    &core->properties.memory,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    engine::ALLOCATOR_GPU_LOCAL_REGION_SIZE,
    "display.allocator_shared"
  );

  lib::gfx::allocator::init(
    &rendering->allocator_host,
    &core->properties.memory,
    VkMemoryPropertyFlagBits(0
      | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    ),
    1024, // this is currently only used for radiance readback, so should be small
    "display.allocator_host"
  );

  engine::datum::probe_radiance::init_ddata(
    &rendering->probe_radiance,
    &rendering->swapchain_description,
    &rendering->allocator_dedicated,
    core
  );

  engine::datum::probe_irradiance::init_ddata(
    &rendering->probe_irradiance,
    &rendering->swapchain_description,
    &rendering->allocator_dedicated,
    core
  );

  engine::datum::probe_attention::init_ddata(
    &rendering->probe_attention,
    &rendering->swapchain_description,
    &rendering->allocator_dedicated,
    core
  );

  engine::display::Data::Common::Stakes common_stakes;
  engine::display::Data::GPass::Stakes gpass_stakes;
  engine::display::Data::LPass::Stakes lpass_stakes;
  { ZoneScopedN(".multi_alloc");
    std::vector<lib::gfx::multi_alloc::Claim> claims;

    rendering->gbuffer.channel0_stakes.resize(swapchain_image_count);
    rendering->gbuffer.channel1_stakes.resize(swapchain_image_count);
    rendering->gbuffer.channel2_stakes.resize(swapchain_image_count);
    rendering->zbuffer.stakes.resize(swapchain_image_count);
    rendering->lbuffer.stakes.resize(swapchain_image_count);
    rendering->final_image.stakes.resize(swapchain_image_count);

    claim_rendering_common(
      swapchain_image_count,
      claims,
      &common_stakes
    );

    claim_rendering_gpass(
      swapchain_image_count,
      claims,
      &gpass_stakes
    );

    claim_rendering_lpass(
      swapchain_image_count,
      claims,
      &lpass_stakes
    );

    for (auto &stake : rendering->gbuffer.channel0_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = GBUFFER_CHANNEL0_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
            ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->gbuffer.channel1_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = GBUFFER_CHANNEL1_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
            ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->gbuffer.channel2_stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = GBUFFER_CHANNEL2_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
            ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->zbuffer.stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ZBUFFER_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_SAMPLED_BIT
            ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->lbuffer.stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = LBUFFER_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = lib::gfx::utilities::mip_levels(
              rendering->swapchain_description.image_extent.width,
              rendering->swapchain_description.image_extent.height
            ),
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_SAMPLED_BIT
              | VK_IMAGE_USAGE_STORAGE_BIT
              | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
              | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          },
        },
        .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .p_stake_image = &stake,
      });
    }
    for (auto &stake : rendering->final_image.stakes) {
      claims.push_back({
        .info = {
          .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = FINAL_IMAGE_FORMAT,
            .extent = {
              .width = rendering->swapchain_description.image_extent.width,
              .height = rendering->swapchain_description.image_extent.height,
              .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = (0
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_STORAGE_BIT
              | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            ),
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
      &session->vulkan.core.properties.basic,
      &session->vulkan.core.properties.memory
    );
  }

  { ZoneScopedN(".final_image_views");
    for (auto stake : rendering->final_image.stakes) {
      VkImageView image_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = FINAL_IMAGE_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &image_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->final_image.views.push_back(image_view);
    }
  }

  { ZoneScopedN(".gbuffer.channel0_views");
    for (auto stake : rendering->gbuffer.channel0_stakes) {
      VkImageView image_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL0_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &image_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->gbuffer.channel0_views.push_back(image_view);
    }
  }

  { ZoneScopedN(".gbuffer.channel1_views");
    for (auto stake : rendering->gbuffer.channel1_stakes) {
      VkImageView image_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL1_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &image_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->gbuffer.channel1_views.push_back(image_view);
    }
  }

  { ZoneScopedN(".gbuffer.channel2_views");
    for (auto stake : rendering->gbuffer.channel2_stakes) {
      VkImageView image_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = GBUFFER_CHANNEL2_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &image_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->gbuffer.channel2_views.push_back(image_view);
    }
  }

  { ZoneScopedN(".zbuffer.views");
    for (auto stake : rendering->zbuffer.stakes) {
      VkImageView depth_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ZBUFFER_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &depth_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->zbuffer.views.push_back(depth_view);
    }
  }

  { ZoneScopedN(".lbuffer.views");
    for (auto stake : rendering->lbuffer.stakes) {
      VkImageView image_view;
      VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = stake.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = LBUFFER_FORMAT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
      };
      {
        auto result = vkCreateImageView(
          vulkan->core.device,
          &create_info,
          vulkan->core.allocator,
          &image_view
        );
        assert(result == VK_SUCCESS);
      }
      rendering->lbuffer.views.push_back(image_view);
    }
  }

  init_rendering_common(
    common_stakes,
    &rendering->common,
    &vulkan->core
  );

  init_rendering_prepass(
    &rendering->prepass,
    &rendering->zbuffer,
    &rendering->swapchain_description,
    &vulkan->prepass,
    &vulkan->core
  );

  init_rendering_gpass(
    &rendering->gpass,
    &rendering->common,
    gpass_stakes,
    &rendering->zbuffer,
    &rendering->gbuffer,
    &rendering->swapchain_description,
    &vulkan->gpass,
    &vulkan->core
  );

  init_rendering_lpass(
    &rendering->lpass,
    lpass_stakes,
    &rendering->common,
    &rendering->swapchain_description,
    &rendering->zbuffer,
    &rendering->gbuffer,
    &rendering->lbuffer,
    &session->vulkan.lpass,
    &session->vulkan.core
  );

  engine::step::probe_measure::init_ddata(
    &rendering->probe_measure,
    &session->vulkan.probe_measure,
    &rendering->common,
    &rendering->lpass.stakes,
    &rendering->probe_radiance,
    &rendering->probe_irradiance,
    &rendering->probe_attention,
    &rendering->swapchain_description,
    &session->vulkan.core
  );

  engine::step::probe_collect::init_ddata(
    &rendering->probe_collect,
    &session->vulkan.probe_collect,
    &rendering->common,
    &rendering->probe_radiance,
    &rendering->probe_irradiance,
    &rendering->probe_attention,
    &rendering->swapchain_description,
    &session->vulkan.core
  );

  engine::step::indirect_light::init_ddata(
    &rendering->indirect_light,
    &session->vulkan.indirect_light,
    &session->vulkan.core,
    &rendering->common,
    &rendering->gbuffer,
    &rendering->zbuffer,
    &rendering->lbuffer,
    &rendering->probe_irradiance,
    &rendering->probe_attention,
    &rendering->swapchain_description
  );

  init_rendering_finalpass(
    &rendering->finalpass,
    &rendering->common,
    &rendering->swapchain_description,
    &rendering->zbuffer,
    &rendering->lbuffer,
    &rendering->final_image,
    &vulkan->finalpass,
    &vulkan->core
  );

  { // radiance readback
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = 8, // see LBuffer format
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    for (size_t i = 0; i < swapchain_image_count; i++) {
      auto buffer = lib::gfx::allocator::create_buffer(
        &rendering->allocator_host,
        core->device,
        core->allocator,
        &core->properties.basic,
        &info
      );
      rendering->readback.radiance_buffers.push_back(buffer);

      auto ptr = (glm::detail::hdata *) lib::gfx::allocator::get_host_mapping(
        &rendering->allocator_host,
        buffer.id
      ).mem;
      rendering->readback.radiance_pointers.push_back(ptr);
    }
  }

  { ZoneScopedN(".imgui_backend");
    { ZoneScopedN(".setup_command_pool");
      VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = session->vulkan.core.queue_family_index,
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
        .format = FINAL_IMAGE_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
      for (size_t i = 0; i < rendering->final_image.views.size(); i++) {
        VkImageView attachments[] = {
          rendering->final_image.views[i],
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
        .QueueFamily = session->vulkan.core.queue_family_index,
        .Queue = session->vulkan.queue_work,
        .DescriptorPool = rendering->common.descriptor_pool,
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
      {
        auto result = vkAllocateCommandBuffers(
          session->vulkan.core.device,
          &alloc_info,
          &cmd_imgui_setup
        );
        assert(result == VK_SUCCESS);
        ZoneValue(uint64_t(cmd_imgui_setup));
      }
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
    engine::frame::schedule_all,
    rendering_yarn_end,
    session.ptr,
    rendering
  );
  auto task_cleanup = defer(
    task::create(
      engine::display::cleanup,
      session_iteration_yarn_end.ptr,
      session.ptr,
      rendering
    )
  );
  auto task_imgui_setup_cleanup = defer(
    task::create(
      rendering_imgui_setup_cleanup,
      &session->vulkan.core,
      &rendering->imgui_backend
    )
  );
  task::inject(ctx->runner, {
    task_frame,
    task_imgui_setup_cleanup.first,
    task_cleanup.first,
  }, {
    .new_dependencies = {
      { signal_imgui_setup_finished, task_frame },
      { signal_imgui_setup_finished, task_imgui_setup_cleanup.first },
      { rendering_yarn_end, task_cleanup.first }
    },
    .changed_parents = { { .ptr = rendering, .children = {
      &rendering->presentation,
      &rendering->presentation_failure_state,
      &rendering->swapchain_description,
      &rendering->imgui_backend,
      &rendering->latest_frame,
      &rendering->command_pools,
      &rendering->descriptor_pools,
      &rendering->graphics_finished_semaphore,
      &rendering->imgui_finished_semaphore,
      &rendering->frame_finished_semaphore,
      &rendering->multi_alloc,
      &rendering->zbuffer,
      &rendering->gbuffer,
      &rendering->lbuffer,
      &rendering->final_image,
      &rendering->common,
      &rendering->prepass,
      &rendering->gpass,
      &rendering->lpass,
      &rendering->finalpass,
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
