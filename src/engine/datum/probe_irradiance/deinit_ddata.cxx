#pragma once
#include <vulkan/vulkan.h>
#include <src/engine/session/data/vulkan.hxx>
#include "data.hxx"

namespace engine::datum::probe_irradiance {

void deinit_ddata(
  DData *it,
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
