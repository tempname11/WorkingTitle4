#include <src/global.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::step::probe_measure {

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
    it->descriptor_set_layout_textures,
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
    it->sampler_albedo,
    core->allocator
  );
  vkDestroySampler(
    core->device,
    it->sampler_probe_irradiance,
    core->allocator
  );
  vkDestroySampler(
    core->device,
    it->sampler_trivial,
    core->allocator
  );
}

} // namespace
