#include <src/global.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::step::probe_collect {

void deinit_sdata(
  SData *it,
  Ref<engine::session::VulkanData::Core> core
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
  vkDestroySampler(
    core->device,
    it->sampler_lbuffer,
    core->allocator
  );
}

} // namespace
