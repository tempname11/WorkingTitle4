#include <src/engine/misc.hxx>
#include <src/engine/uploader.hxx>
#include <src/engine/blas_storage.hxx>
#include <src/engine/system/entities/impl.hxx>
#include <src/engine/component/artline_model.hxx>
#include <src/engine/component/base_transform.hxx>
#include "generate_render_list.hxx"

namespace engine::frame {

void generate_render_list(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<session::Data> session,
  Use<session::VulkanData::Meshes> meshes,
  Use<session::VulkanData::Textures> textures,
  Use<system::entities::Impl> entities,
  Use<component::artline_model::storage_t> cmp_artline_model,
  Use<component::base_transform::storage_t> cmp_base_transform,
  Own<misc::RenderList> render_list
) {
  ZoneScoped;
  for (size_t i = 0; i < entities->types->count; i++) {
    auto ty = &entities->types->data[i];
    
    auto mask = (0
      | (1 << uint64_t(component::type_t::artline_model))
      | (1 << uint64_t(component::type_t::base_transform))
    );

    if ((ty->component_bit_mask & mask) != mask) {
      continue;
    }

    for (size_t j = 0; j < ty->indices->count; j += ty->component_count) {
      auto artline_model = &cmp_artline_model->values->data[
        ty->indices->data[
          j + ty->component_index_offsets[uint8_t(component::type_t::artline_model)]
        ]
      ];

      auto base_transform = &cmp_base_transform->values->data[
        ty->indices->data[
          j + ty->component_index_offsets[uint8_t(component::type_t::base_transform)]
        ]
      ];

      {
        lib::mutex::lock(&artline_model->model->mutex);
        for (size_t k = 0; k < artline_model->model->parts->count; k++) {
          auto part = &artline_model->model->parts->data[k];

          auto &mesh = meshes->items.at(part->mesh_id);
          auto &albedo_uploader_id = textures->items.at(part->texture_albedo_id).id;
          auto &normal_uploader_id = textures->items.at(part->texture_normal_id).id;
          auto &romeao_uploader_id = textures->items.at(part->texture_romeao_id).id;

          // this is very badly designed :UploaderMustBeImproved
          // i.e. we'll take multiple mutexes for each item!
          // also, there is double indirection, where there should not be!
          auto pair = engine::uploader::get_buffer(
            session->vulkan->uploader,
            mesh.id
          );
          auto mesh_buffer = pair.first;
          auto mesh_buffer_address = pair.second;

          auto albedo_view = engine::uploader::get_image(
            session->vulkan->uploader,
            albedo_uploader_id
          ).second;

          auto normal_view = engine::uploader::get_image(
            session->vulkan->uploader,
            normal_uploader_id
          ).second;

          auto romeao_view = engine::uploader::get_image(
            session->vulkan->uploader,
            romeao_uploader_id
          ).second;

          auto blas_address = engine::blas_storage::get_address(
            session->vulkan->blas_storage,
            mesh.blas_id
          );

          render_list->items.push_back(engine::misc::RenderList::Item {
            .transform = base_transform->matrix * part->transform,
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

        lib::mutex::unlock(&artline_model->model->mutex);
      }
    }
  }
}

} // namespace
