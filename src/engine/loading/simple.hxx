#pragma once
#include <src/lib/task.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::simple {

void load(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  lib::gpu_signal::Support *gpu_signal_support,
  VkQueue *queue_work,
  SessionData::Vulkan::Meshes *meshes,
  SessionData::Vulkan::Textures *textures
);

void unload(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  RenderingData::InflightGPU *inflight_gpu,
  SessionData::Vulkan::Meshes *meshes,
  SessionData::Vulkan::Textures *textures
);

} // namespace
