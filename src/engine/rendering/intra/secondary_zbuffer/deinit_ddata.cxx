#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::rendering::intra::secondary_zbuffer {

void deinit_ddata(
  DData *it,
  Use<engine::session::Vulkan::Core> core
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
