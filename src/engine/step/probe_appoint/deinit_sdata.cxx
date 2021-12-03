#include <src/global.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::step::probe_appoint {

void deinit_sdata(
  SData *it,
  Ref<engine::session::VulkanData::Core> core
) {
  ZoneScoped;

  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_frame,
    core->allocator
  );
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_cascade,
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
