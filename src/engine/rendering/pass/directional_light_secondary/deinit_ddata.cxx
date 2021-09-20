#include <src/global.hxx>
#include <src/engine/session.hxx>
#include "data.hxx"

namespace engine::rendering::pass::directional_light_secondary {

void deinit_ddata(
  DData *it,
  Use<SessionData::Vulkan::Core> core
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
