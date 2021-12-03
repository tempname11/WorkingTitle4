#include <src/global.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::step::indirect_light {

void deinit_ddata(
  DData *it,
  Ref<engine::session::VulkanData::Core> core
) {
  ZoneScoped;

  for (auto framebuffer : it->framebuffers) {
    vkDestroyFramebuffer(
      core->device,
      framebuffer,
      core->allocator
    );
  }
}

} // namespace
