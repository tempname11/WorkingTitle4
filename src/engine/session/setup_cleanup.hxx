#pragma once
#include <src/engine/session/data.hxx>

void session_setup_cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::SetupData> data,
  Ref<engine::session::Vulkan::Core> core
);
