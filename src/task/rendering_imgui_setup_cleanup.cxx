#include <backends/imgui_impl_vulkan.h>
#include "rendering_imgui_setup_cleanup.hxx"

TASK_DECL {
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
