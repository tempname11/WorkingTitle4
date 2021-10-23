#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/gfx/command_pool_2.hxx>
#include <src/lib/gfx/multi_alloc.hxx>
#include <src/engine/rendering/intra/probe_radiance/data.hxx>
#include <src/engine/rendering/intra/probe_irradiance/data.hxx>
#include <src/engine/rendering/intra/probe_attention/data.hxx>
#include <src/engine/rendering/pass/probe_measure/data.hxx>
#include <src/engine/rendering/pass/probe_collect/data.hxx>
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

    #ifdef ENGINE_DEVELOPER
      struct Directives {
        bool should_capture_screenshot;
        std::string screenshot_path;
      } directives;
    #endif
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
  lib::gfx::Allocator allocator_host;

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

  rendering::intra::probe_radiance::DData probe_radiance;
  rendering::intra::probe_irradiance::DData probe_irradiance;
  rendering::intra::probe_attention::DData probe_attention;

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

  engine::rendering::pass::probe_measure::DData pass_probe_measure;
  engine::rendering::pass::probe_collect::DData pass_probe_collect;
  engine::rendering::pass::indirect_light::DData pass_indirect_light;

  struct Finalpass {
    std::vector<VkDescriptorSet> descriptor_sets;
  } finalpass;

  struct Readback {
    std::vector<lib::gfx::allocator::Buffer> radiance_buffers;
    std::vector<glm::detail::hdata *> radiance_pointers;
  } readback;
};

} // namespace