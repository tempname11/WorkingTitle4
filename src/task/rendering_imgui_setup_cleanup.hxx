#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

void rendering_imgui_setup_cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<engine::session::Vulkan::Core> core,
  Own<engine::display::Data::ImguiBackend> imgui_backend
);
