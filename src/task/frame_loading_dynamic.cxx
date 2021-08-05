#include <src/engine/loading/mesh.hxx>
#include <src/engine/loading/group.hxx>
#include "frame_loading_dynamic.hxx"

TASK_DECL {
  ZoneScoped;
  if (imgui_reactions->reloaded_mesh_id != 0) {
    auto id = imgui_reactions->reloaded_mesh_id;
    auto group = &meta_meshes->items.at(id);
    if (group->status == SessionData::MetaMeshes::Status::Ready) {
      engine::loading::mesh::reload(
        id,
        ctx,
        session,
        meta_meshes,
        inflight_gpu
      );
    }
  }

  if (imgui_reactions->created_group_description != nullptr) {
    lib::GUID group_id;
    engine::loading::group::create(
      ctx,
      session,
      imgui_reactions->created_group_description,
      &group_id
    );
    delete imgui_reactions->created_group_description;
  }

  if (imgui_reactions->added_item_to_group_description != nullptr) {
    engine::loading::group::add_item(
      ctx,
      imgui_reactions->added_item_to_group_id,
      nullptr,
      imgui_reactions->added_item_to_group_description,
      session
    );
    delete imgui_reactions->added_item_to_group_description;
  }

  if (imgui_reactions->removed_group_id != 0) {
    engine::loading::group::remove(
      ctx,
      imgui_reactions->removed_group_id,
      session,
      groups,
      inflight_gpu
    );
  }
}
