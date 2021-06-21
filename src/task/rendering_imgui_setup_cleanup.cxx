#include <backends/imgui_impl_vulkan.h>
#include "task.hxx"

void rendering_imgui_setup_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<RenderingData::ImguiBackend> imgui_backend
) {
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
