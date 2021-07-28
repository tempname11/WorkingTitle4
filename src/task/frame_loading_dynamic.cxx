#include <src/engine/loading/mesh.hxx>
#include "frame_loading_dynamic.hxx"

TASK_DECL {
  ZoneScoped;
  if (imgui_reactions->reload_mesh_id != 0) {
    auto id = imgui_reactions->reload_mesh_id;
    auto group = &meta_meshes->items.at(id);
    if (group->status == SessionData::MetaMeshes::Status::Ready) {
      engine::loading::mesh::reload(
        id,
        ctx,
        session,
        unfinished_yarns,
        meta_meshes,
        inflight_gpu
      );
    }
  }
}
