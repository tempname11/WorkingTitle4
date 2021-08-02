
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_generate_render_list( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Scene> scene, \
  usage::Some<SessionData::Vulkan::Meshes> meshes, \
  usage::Some<SessionData::Vulkan::Textures> textures, \
  usage::Full<engine::misc::RenderList> render_list \
)

TASK_DECL;
