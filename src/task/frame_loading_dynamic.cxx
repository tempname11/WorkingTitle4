#include <src/engine/loading/mesh.hxx>
#include <src/engine/loading/group.hxx>
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
  if (imgui_reactions->load_group_description != nullptr) {
    engine::loading::group::add_simple(
      ctx,
      groups,
      guid_counter,
      unfinished_yarns,
      session,
      imgui_reactions->load_group_description
    );

    delete imgui_reactions->load_group_description;
  }
  if (imgui_reactions->removed_group_id != 0) {
    engine::loading::group::remove(
      ctx,
      imgui_reactions->removed_group_id,
      session,
      groups,
      unfinished_yarns,
      inflight_gpu
    );
  }
}
