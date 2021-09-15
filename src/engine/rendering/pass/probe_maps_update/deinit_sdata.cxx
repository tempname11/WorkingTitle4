#include <src/global.hxx>
#include <src/engine/session.hxx>
#include "data.hxx"

namespace engine::rendering::pass::probe_maps_update {

void deinit_sdata(
  SData *it,
  Use<SessionData::Vulkan::Core> core
) {
  ZoneScoped;

  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout,
    core->allocator
  );
  vkDestroyPipelineLayout(
    core->device,
    it->pipeline_layout,
    core->allocator
  );
  vkDestroyPipeline(
    core->device,
    it->pipeline,
    core->allocator
  );
}

} // namespace
