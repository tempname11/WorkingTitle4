#pragma once
#include <vulkan/vulkan.h>
#include <TracyVulkan.hpp>
#include <src/engine/step/probe_measure/data.hxx>
#include <src/engine/step/probe_collect/data.hxx>
#include <src/engine/step/indirect_light/data.hxx>

namespace engine::session {

struct Vulkan : lib::task::ParentResource {
  bool ready;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR window_surface;
  VkPhysicalDevice physical_device;

  struct Core {
    VkDevice device;
    const VkAllocationCallbacks *allocator;
    TracyVkCtx tracy_context;

    struct Properties {
      VkPhysicalDeviceProperties basic;
      VkPhysicalDeviceMemoryProperties memory;
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing;
    } properties;

    struct ExtensionPointers {
      PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
      PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
      PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
      PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
      PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    } extension_pointers;

    uint32_t queue_family_index;
  } core;

  VkQueue queue_present;
  VkQueue queue_work; // graphics, compute, transfer

  VkCommandPool tracy_setup_command_pool;

  using MultiAlloc = lib::gfx::multi_alloc::Instance;
  MultiAlloc multi_alloc;

  using Uploader = engine::Uploader;
  Uploader uploader;

  using BlasStorage = engine::BlasStorage;
  BlasStorage blas_storage;

  struct Meshes {
    struct Item {
      engine::uploader::ID id;
      engine::blas_storage::ID blas_id;
      uint32_t index_count;
      uint32_t buffer_offset_indices;
      uint32_t buffer_offset_vertices;
    };
    std::unordered_map<lib::GUID, Item> items;
  } meshes;

  struct Textures {
    struct Item {
      engine::uploader::ID id;
    };
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
    VkSampler sampler_normal;
    VkSampler sampler_biased;
  } gpass;

  struct LPass {
    VkDescriptorSetLayout descriptor_set_layout_frame;
    VkDescriptorSetLayout descriptor_set_layout_directional_light;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline pipeline_sun;
  } lpass;

  engine::step::probe_measure::SData probe_measure;
  engine::step::probe_collect::SData probe_collect;
  engine::step::indirect_light::SData indirect_light;

  struct Finalpass {
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkSampler sampler_lbuffer;
  } finalpass;
};

} // namespace

