#include <src/engine/loading/texture.hxx>
#include <src/engine/loading/mesh.hxx>
#include <src/engine/loading/group.hxx>
#include "frame_loading_dynamic.hxx"

TASK_DECL {
  ZoneScoped;
  if (imgui_reactions->reloaded_mesh_id != 0) {
    auto id = imgui_reactions->reloaded_mesh_id;
    auto group = &meta_meshes->items.at(id);
    if (group->status == engine::session::Data::MetaMeshes::Status::Ready) {
      engine::loading::mesh::reload(
        id,
        ctx,
        session,
        meta_meshes
      );
    }
  }

  if (imgui_reactions->reloaded_texture_id != 0) {
    auto id = imgui_reactions->reloaded_texture_id;
    auto group = &meta_textures->items.at(id);
    if (group->status == engine::session::Data::MetaTextures::Status::Ready) {
      engine::loading::texture::reload(
        id,
        ctx,
        session,
        meta_textures
      );
    }
  }

  if (imgui_reactions->created_group_description != nullptr) {
    engine::loading::group::create(
      ctx,
      session,
      imgui_reactions->created_group_description
    );
    delete imgui_reactions->created_group_description;
  }

  if (imgui_reactions->added_item_to_group_description != nullptr) {
    // @Incomplete: no transform in GUI yet.
    imgui_reactions->added_item_to_group_description->transform = glm::mat4(1.0);
    /*glm::translate(
      glm::rotate(
        glm::mat4(1.0),
        glm::radians(45.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
      ),
      glm::vec3(0.0f, -32.0f, 0.0f)
    );*/

    engine::loading::group::add_item(
      ctx,
      imgui_reactions->added_item_to_group_id,
      imgui_reactions->added_item_to_group_description,
      session
    );
    delete imgui_reactions->added_item_to_group_description;
  }

  if (imgui_reactions->removed_group_id != 0) {
    engine::loading::group::deref(
      ctx,
      imgui_reactions->removed_group_id,
      session
    );
  }
}
