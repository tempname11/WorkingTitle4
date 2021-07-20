#pragma once
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_loading_dynamic( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::None<SessionData::UnfinishedYarns> unfinished_yarns, \
  usage::None<SessionData::Scene> scene, \
  usage::None<SessionData::Vulkan::Core> core, \
  usage::Some<lib::gpu_signal::Support> gpu_signal_support, \
  usage::Full<VkQueue> queue_work, \
  usage::Some<RenderingData::InflightGPU> inflight_gpu, \
  usage::Some<SessionData::Vulkan::Meshes> meshes, \
  usage::Some<SessionData::Vulkan::Textures> textures, \
  usage::Some<engine::misc::ImguiReactions> imgui_reactions \
)

TASK_DECL;
