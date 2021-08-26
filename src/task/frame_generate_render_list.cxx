#include <src/engine/uploader.hxx>
#include "frame_generate_render_list.hxx"

TASK_DECL {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items.at(item.mesh_id);
    auto &albedo_uploader_id = textures->items.at(item.texture_albedo_id).id;
    auto &normal_uploader_id = textures->items.at(item.texture_normal_id).id;
    auto &romeao_uploader_id = textures->items.at(item.texture_romeao_id).id;

    // @Rushed: this is very badly designed,
    // i.e. we'll take multiple mutexes for each item!
    // also, there is double indirection, where there should not be!
    auto buffer = engine::uploader::get_buffer(
      &session->vulkan.uploader,
      mesh.id
    );

    auto albedo_view = engine::uploader::get_image(
      &session->vulkan.uploader,
      albedo_uploader_id
    ).second;

    auto normal_view = engine::uploader::get_image(
      &session->vulkan.uploader,
      normal_uploader_id
    ).second;

    auto romeao_view = engine::uploader::get_image(
      &session->vulkan.uploader,
      romeao_uploader_id
    ).second;

    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = buffer,
      .mesh_index_count = mesh.index_count,
      .mesh_buffer_offset_indices = mesh.buffer_offset_indices,
      .mesh_buffer_offset_vertices = mesh.buffer_offset_vertices,
      .texture_albedo_view = albedo_view,
      .texture_normal_view = normal_view,
      .texture_romeao_view = romeao_view,
    });
  }
}
