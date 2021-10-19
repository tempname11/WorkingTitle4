#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>

namespace engine::frame {

void imgui_new_frame(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::Data::ImguiContext> imgui,
  Own<engine::display::Data::ImguiBackend> imgui_backend,
  Own<engine::session::Data::GLFW> glfw
);

} // namespace
