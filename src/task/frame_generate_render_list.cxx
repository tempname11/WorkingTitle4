#include <src/engine/uploader.hxx>
#include "frame_generate_render_list.hxx"

TASK_DECL {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items.at(item.mesh_id);
    auto &albedo = textures->items.at(item.texture_albedo_id);
    auto &normal = textures->items.at(item.texture_normal_id);
    auto &romeao = textures->items.at(item.texture_romeao_id);

    // @Temporary: this is very badly designed,
    // we'll take a mutex for each item!
    auto buffer = engine::uploader::get_buffer(
      &session->vulkan.uploader,
      mesh.id
    );
    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = buffer,
      .mesh_vertex_count = mesh.vertex_count,
      .texture_albedo_view = albedo.view,
      .texture_normal_view = normal.view,
      .texture_romeao_view = romeao.view,
    });
  }
}
