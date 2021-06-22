#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>
#include <src/lib/task.hxx>
#include <src/lib/gfx/command_pool_2.hxx>
#include <src/lib/gfx/multi_alloc.hxx>

struct RenderingData : lib::task::ParentResource {
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
    std::vector<lib::task::Task *> signals;
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

  struct FinalImage {
    std::vector<lib::gfx::multi_alloc::StakeImage> stakes;
    std::vector<VkImageView> views;
  } final_image;

  VkSemaphore example_finished_semaphore;
  VkSemaphore imgui_finished_semaphore;
  VkSemaphore frame_rendered_semaphore;

  lib::gfx::multi_alloc::Instance multi_alloc;

  struct Example {
    struct Constants {
      uint32_t vs_ubo_aligned_size;
      uint32_t fs_ubo_aligned_size;
      uint32_t total_ubo_aligned_size;
    } constants;
    lib::gfx::multi_alloc::StakeBuffer uniform_stake;
    std::vector<lib::gfx::multi_alloc::StakeImage> gbuffer_channel0_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> gbuffer_channel1_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> gbuffer_channel2_stakes;
    std::vector<lib::gfx::multi_alloc::StakeImage> gbuffer_depth_stakes;
    std::vector<VkImageView> gbuffer_channel0_views;
    std::vector<VkImageView> gbuffer_channel1_views;
    std::vector<VkImageView> gbuffer_channel2_views;
    std::vector<VkImageView> gbuffer_depth_views;
    struct Prepass {
      VkRenderPass render_pass;
      VkPipeline pipeline;
      std::vector<VkFramebuffer> framebuffers;
    } prepass;
    struct GPass {
      VkRenderPass render_pass;
      VkPipeline pipeline;
      std::vector<VkFramebuffer> framebuffers;
    } gpass;
  } example;
};