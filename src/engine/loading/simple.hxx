#pragma once
#include <src/lib/task.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::simple {

void load(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  SessionData::Vulkan::Meshes *meshes
);

void unload(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  RenderingData::InflightGPU *inflight_gpu,
  SessionData::Vulkan::Meshes *meshes
);

} // namespace
