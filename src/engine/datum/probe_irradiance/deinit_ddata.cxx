#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::datum::probe_irradiance {

void deinit_ddata(
  DData *it,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  vkDestroyImageView(
    core->device,
    it->view,
    core->allocator
  );
}

} // namespace
