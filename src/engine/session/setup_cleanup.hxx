#pragma once
#include <src/engine/session/data.hxx>
#include <src/engine/session/data/vulkan.hxx>

void session_setup_cleanup(
  lib::task::Context<QUEUE_INDEX_MAIN_THREAD_ONLY> *ctx,
  Own<engine::session::SetupData> data,
  Ref<engine::session::VulkanData::Core> core
);
