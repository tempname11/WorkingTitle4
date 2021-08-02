#include "frame_generate_render_list.hxx"

TASK_DECL {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items.at(item.mesh_id);
    auto &albedo = textures->items.at(item.texture_albedo_id);
    auto &normal = textures->items.at(item.texture_normal_id);
    auto &romeao = textures->items.at(item.texture_romeao_id);
     DBG("push {} for rlist {}", (void *) mesh.vertex_stake.buffer, (void*) render_list.ptr);
    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = mesh.vertex_stake.buffer,
      .mesh_vertex_count = uint32_t(mesh.triangle_count * 3),
      .texture_albedo_view = albedo.view,
      .texture_normal_view = normal.view,
      .texture_romeao_view = romeao.view,
    });
  }
}
