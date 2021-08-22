#pragma once
#include <src/global.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_generate_render_list( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Use<SessionData::Scene> scene, \
  Use<SessionData::Vulkan::Meshes> meshes, \
  Use<SessionData::Vulkan::Textures> textures, \
  Own<engine::misc::RenderList> render_list \
)

TASK_DECL;
