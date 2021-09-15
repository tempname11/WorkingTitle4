#pragma once
#include <src/global.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session.hxx>

namespace engine::rendering::intra::secondary_gbuffer {

struct DData;

void init_ddata(
  DData *out,
  Use<display::Data::SwapchainDescription> swapchain_description,
  Ref<lib::gfx::Allocator> allocator_dedicated,
  Use<SessionData::Vulkan::Core> core
);

void deinit_ddata(
  DData *it,
  Use<SessionData::Vulkan::Core> core
);

} // namespace