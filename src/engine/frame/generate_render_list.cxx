#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include "generate_render_list.hxx"

namespace engine::frame {

void generate_render_list(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::Scene> scene,
  Use<engine::session::Vulkan::Meshes> meshes,
  Use<engine::session::Vulkan::Textures> textures,
  Own<engine::misc::RenderList> render_list
) {
  ZoneScoped;
  for (auto &item : scene->items) {
    auto &mesh = meshes->items.at(item.mesh_id);
    auto &albedo_uploader_id = textures->items.at(item.texture_albedo_id).id;
    auto &normal_uploader_id = textures->items.at(item.texture_normal_id).id;
    auto &romeao_uploader_id = textures->items.at(item.texture_romeao_id).id;

    // this is very badly designed :UploaderMustBeImproved
    // i.e. we'll take multiple mutexes for each item!
    // also, there is double indirection, where there should not be!
    auto pair = engine::uploader::get_buffer(
      &session->vulkan.uploader,
      mesh.id
    );
    auto mesh_buffer = pair.first;
    auto mesh_buffer_address = pair.second;

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

    auto blas_address = engine::blas_storage::get_address(
      &session->vulkan.blas_storage,
      mesh.blas_id
    );

    render_list->items.push_back(engine::misc::RenderList::Item {
      .transform = item.transform,
      .mesh_buffer = mesh_buffer,
      .mesh_index_count = mesh.index_count,
      .mesh_buffer_offset_indices = mesh.buffer_offset_indices,
      .mesh_buffer_offset_vertices = mesh.buffer_offset_vertices,
      .mesh_buffer_address = mesh_buffer_address,
      .blas_address = blas_address,
      .texture_albedo_view = albedo_view,
      .texture_normal_view = normal_view,
      .texture_romeao_view = romeao_view,
    });
  }
}

} // namespace
