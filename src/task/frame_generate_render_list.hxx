#pragma once
#include <src/global.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_generate_render_list( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<engine::session::Data> session, \
  Use<engine::session::Data::Scene> scene, \
  Use<engine::session::Vulkan::Meshes> meshes, \
  Use<engine::session::Vulkan::Textures> textures, \
  Own<engine::misc::RenderList> render_list \
)

TASK_DECL;
