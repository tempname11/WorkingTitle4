#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::indirect_light_secondary {

void deinit_ddata(
  DData *it,
  Use<engine::session::Vulkan::Core> core
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
