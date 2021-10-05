#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/gfx/command_pool_2.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/engine/rendering/intra/secondary_zbuffer/data.hxx>
#include <src/engine/rendering/intra/secondary_gbuffer/data.hxx>
#include <src/engine/rendering/intra/secondary_lbuffer/data.hxx>
#include <src/engine/rendering/intra/probe_light_map/data.hxx>
#include <src/engine/rendering/intra/probe_depth_map/data.hxx>
#include <src/engine/rendering/pass/secondary_geometry/data.hxx>
#include <src/engine/rendering/pass/indirect_light_secondary/data.hxx>
#include <src/engine/rendering/pass/directional_light_secondary/data.hxx>
#include <src/engine/rendering/pass/probe_light_update/data.hxx>
#include <src/engine/rendering/pass/probe_depth_update/data.hxx>
#include <src/engine/rendering/pass/indirect_light/data.hxx>
#include <src/engine/common/shared_descriptor_pool.hxx>

namespace engine::display {

struct Data : lib::task::ParentResource {
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

  struct ImguiBackend {
    // ImGui_ImplVulkan_*
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool setup_command_pool;
    VkSemaphore setup_semaphore;
  } imgui_backend;

  struct FrameInfo {
    uint64_t timestamp_ns;
    uint64_t elapsed_ns;

    uint64_t number;
    uint64_t timeline_semaphore_value;
    uint8_t inflight_index;
    bool is_sequential;
  } latest_frame;

  typedef std::vector<CommandPool2> CommandPools;
  CommandPools command_pools;

  typedef std::vector<common::SharedDescriptorPool> DescriptorPools;
  DescriptorPools descriptor_pools;

  VkSemaphore graphics_finished_semaphore;
  VkSemaphore imgui_finished_semaphore;
  VkSemaphore frame_finished_semaphore;

  // @Note: these are mutex-protected, but they are actually only used in init/deinit and 
  // we could just skip the protection. does this cost much? something to think about.
  lib::gfx::Allocator allocator_dedicated;
  lib::gfx::Allocator allocator_shared;

  lib::gfx::multi_alloc::Instance multi_alloc; // now deprecated

  struct ZBuffer {
    std::vector<lib::gfx::multi_alloc::StakeImage> stakes;
    std::vector<VkImageView> views;
  } zbuffer;

  struct GBuffer {
    std::vector<lib::gfx::multi_alloc::StakeImage> channel0_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> channel1_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> channel2_stakes;
    std::vector<VkImageView> channel0_views;
    std::vector<VkImageView> channel1_views;
    std::vector<VkImageView> channel2_views;
  } gbuffer;

  struct LBuffer {
    std::vector<lib::gfx::multi_alloc::StakeImage> stakes;
    std::vector<VkImageView> views;
  } lbuffer;

  rendering::intra::secondary_zbuffer::DData zbuffer2;
  rendering::intra::secondary_gbuffer::DData gbuffer2;
  rendering::intra::secondary_lbuffer::DData lbuffer2;
  rendering::intra::probe_light_map::DData probe_light_map;
  rendering::intra::probe_depth_map::DData probe_depth_map;

  struct FinalImage {
    std::vector<lib::gfx::multi_alloc::StakeImage> stakes;
    std::vector<VkImageView> views;
  } final_image;

  struct Common {
    VkDescriptorPool descriptor_pool;

    struct Stakes {
      std::vector<lib::gfx::multi_alloc::StakeBuffer> ubo_frame;
    } stakes;
  } common;

  struct Prepass {
    std::vector<VkFramebuffer> framebuffers;
  } prepass;

  struct GPass {
    struct Stakes {} stakes;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkDescriptorSet> descriptor_sets_frame;
  } gpass;

  struct LPass {
    // remove for :ManyLights
    struct Stakes {
      std::vector<lib::gfx::multi_alloc::StakeBuffer> ubo_directional_light;
    } stakes;

    std::vector<VkDescriptorSet> descriptor_sets_frame;

    // remove for :ManyLights
    std::vector<VkDescriptorSet> descriptor_sets_directional_light;

    std::vector<VkFramebuffer> framebuffers;
  } lpass;

  engine::rendering::pass::secondary_geometry::DData pass_secondary_geometry;
  engine::rendering::pass::indirect_light_secondary::DData pass_indirect_light_secondary;
  engine::rendering::pass::directional_light_secondary::DData pass_directional_light_secondary;
  engine::rendering::pass::probe_light_update::DData pass_probe_light_update;
  engine::rendering::pass::probe_depth_update::DData pass_probe_depth_update;
  engine::rendering::pass::indirect_light::DData pass_indirect_light;

  struct Finalpass {
    std::vector<VkDescriptorSet> descriptor_sets;
  } finalpass;
};

} // namespace