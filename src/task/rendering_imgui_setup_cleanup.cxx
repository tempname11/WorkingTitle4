#include <backends/imgui_impl_vulkan.h>
#include "rendering_imgui_setup_cleanup.hxx"

void rendering_imgui_setup_cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::ImguiBackend> imgui_backend
) {
  ZoneScoped;

  ImGui_ImplVulkan_DestroyFontUploadObjects();
  vkDestroyCommandPool(
    core->device,
    imgui_backend->setup_command_pool,
    core->allocator
  );
  vkDestroySemaphore(
    core->device,
    imgui_backend->setup_semaphore,
    core->allocator
  );
}
