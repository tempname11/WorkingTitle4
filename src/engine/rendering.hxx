#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/gfx/command_pool_2.hxx>
#include <src/lib/gfx/multi_alloc.hxx>

struct DescriptorPool {
  std::mutex mutex;
  VkDescriptorPool pool;
};

const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
const VkFormat ZBUFFER_FORMAT = VK_FORMAT_D32_SFLOAT;
const VkFormat GBUFFER_CHANNEL0_FORMAT = VK_FORMAT_R16G16B16A16_SNORM;
const VkFormat GBUFFER_CHANNEL1_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
const VkFormat GBUFFER_CHANNEL2_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
const VkFormat LBUFFER_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
const VkFormat FINAL_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;

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

  typedef std::vector<DescriptorPool> DescriptorPools;
  DescriptorPools descriptor_pools;

  VkSemaphore graphics_finished_semaphore;
  VkSemaphore imgui_finished_semaphore;
  VkSemaphore frame_finished_semaphore;

  lib::gfx::multi_alloc::Instance multi_alloc;

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
    struct Stakes {
      std::vector<lib::gfx::multi_alloc::StakeBuffer> ubo_material;
    } stakes;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkDescriptorSet> descriptor_sets_frame;
  } gpass;

  struct LPass {
    struct Stakes {
      std::vector<lib::gfx::multi_alloc::StakeBuffer> ubo_directional_light;
    } stakes;

    std::vector<VkDescriptorSet> descriptor_sets_frame;
    std::vector<VkDescriptorSet> descriptor_sets_directional_light;
    std::vector<VkFramebuffer> framebuffers;
  } lpass;

  struct Finalpass {
    std::vector<VkDescriptorSet> descriptor_sets;
  } finalpass;
};