#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::datum::probe_offsets {

void deinit_sdata(
  SData *it,
  Ref<engine::session::VulkanData::Core> core
) {
  ZoneScoped;

  vkDestroyImageView(
    core->device,
    it->view,
    core->allocator
  );
}

} // namespace
