#include <src/global.hxx>
#include <src/embedded.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "data.hxx"

namespace engine::rendering::pass::probe_maps_update {

void init_ddata(
  DData *out,
  Use<SData> sdata,
  Own<display::Data::Common> common,
  Use<intra::probe_light_map::DData> probe_light_map,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<SessionData::Vulkan::Core> core
) {
  ZoneScoped;

  std::vector<VkDescriptorSet> descriptor_sets;
  descriptor_sets.resize(swapchain_description->image_count);
  std::vector<VkDescriptorSetLayout> layouts(swapchain_description->image_count);
  for (auto &layout : layouts) {
    layout = sdata->descriptor_set_layout;
  }
  {
    VkDescriptorSetAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = common->descriptor_pool,
      .descriptorSetCount = swapchain_description->image_count,
      .pSetLayouts = layouts.data(),
    };
    auto result = vkAllocateDescriptorSets(
      core->device,
      &allocate_info,
      descriptor_sets.data()
    );
    assert(result == VK_SUCCESS);
  }

  for (size_t i = 0; i < swapchain_description->image_count; i++) {
    VkDescriptorImageInfo probe_light_map_info = {
      .imageView = probe_light_map->views[i],
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet writes[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &probe_light_map_info,
      },
    };
    vkUpdateDescriptorSets(
      core->device,
      sizeof(writes) / sizeof(*writes), writes,
      0, nullptr
    );
  }

  *out = {
    .descriptor_sets = std::move(descriptor_sets),
  };
}

} // namespace
