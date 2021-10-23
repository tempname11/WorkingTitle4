#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::probe_collect {

void deinit_sdata(
  SData *it,
  Use<engine::session::Vulkan::Core> core
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
