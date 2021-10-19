#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::directional_light_secondary {

void deinit_sdata(
  SData *it,
  Use<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_frame,
    core->allocator
  );
  vkDestroyDescriptorSetLayout(
    core->device,
    it->descriptor_set_layout_light,
    core->allocator
  );
  vkDestroyPipelineLayout(
    core->device,
    it->pipeline_layout,
    core->allocator
  );
  vkDestroyRenderPass(
    core->device,
    it->render_pass,
    core->allocator
  );
  vkDestroyPipeline(
    core->device,
    it->pipeline,
    core->allocator
  );
}

} // namespace
