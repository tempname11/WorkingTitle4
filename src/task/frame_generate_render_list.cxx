#include "frame_generate_render_list.hxx"

TASK_DECL {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items[item.mesh_index];
    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = mesh.data.vertex_stake.buffer,
      .mesh_vertex_count = uint32_t(mesh.data.triangle_count * 3),
    });
  }
}
