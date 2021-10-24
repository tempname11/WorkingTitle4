#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::datum::probe_radiance {

void deinit_ddata(
  DData *it,
  Ref<engine::session::Vulkan::Core> core
) {
  ZoneScoped;

  for (auto view : it->views) {
    vkDestroyImageView(
      core->device,
      view,
      core->allocator
    );
  }
}

} // namespace
