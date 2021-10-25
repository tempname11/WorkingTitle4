#pragma once
#include <src/engine/session/data.hxx>

namespace engine::display {

void setup(
  lib::task::ContextBase *ctx,
  Ref<lib::Task> session_iteration_yarn_end,
  Ref<engine::session::Data> session,
  VkSurfaceCapabilitiesKHR *surface_capabilities
);

} // namespace
