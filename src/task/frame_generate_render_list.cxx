#include "frame_generate_render_list.hxx"

TASK_DECL {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items[item.mesh_index];
    auto &albedo = textures->items[item.texture_albedo_index];
    auto &normal = textures->items[item.texture_normal_index];
    auto &romeao = textures->items[item.texture_romeao_index];
    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = mesh.data.vertex_stake.buffer,
      .mesh_vertex_count = uint32_t(mesh.data.triangle_count * 3),
      .texture_albedo_view = albedo.data.view,
      .texture_normal_view = normal.data.view,
      .texture_romeao_view = romeao.data.view,
    });
  }
}
