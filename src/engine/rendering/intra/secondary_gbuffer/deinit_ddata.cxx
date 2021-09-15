#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include "data.hxx"

namespace engine::rendering::intra::secondary_gbuffer {

void deinit_ddata(
  DData *it,
  Use<SessionData::Vulkan::Core> core
) {
  ZoneScoped;

  for (auto view : it->channel0_views) {
    vkDestroyImageView(
      core->device,
      view,
      core->allocator
    );
  }

  for (auto view : it->channel1_views) {
    vkDestroyImageView(
      core->device,
      view,
      core->allocator
    );
  }

  for (auto view : it->channel2_views) {
    vkDestroyImageView(
      core->device,
      view,
      core->allocator
    );
  }
}

} // namespace
