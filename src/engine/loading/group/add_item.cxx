#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/task/defer.hxx>
#include <src/engine/texture.hxx>
#include "../mesh.hxx"
#include "../texture.hxx"
#include "../group.hxx"

namespace engine::loading::group {

struct AddItemData {
  lib::GUID group_id;

  lib::GUID mesh_id;
  lib::GUID albedo_id;
  lib::GUID normal_id;
  lib::GUID romeao_id;
};

void _add_item_insert(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<SessionData::Scene> scene,
  Own<AddItemData> data 
) {
  ZoneScoped;
  scene->items.push_back(SessionData::Scene::Item {
    .group_id = data->group_id,
    .transform = glm::translate(glm::mat4(1.0f), glm::vec3(
      // @Temporary
      float(rand()) / RAND_MAX * 10.0f,
      float(rand()) / RAND_MAX * 10.0f,
      float(rand()) / RAND_MAX * 10.0f
    )),
    .mesh_id = data->mesh_id,
    .texture_albedo_id = data->albedo_id,
    .texture_normal_id = data->normal_id,
    .texture_romeao_id = data->romeao_id,
  });

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  delete data.ptr;
}

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  lib::Task *wait_for_group,
  ItemDescription *desc,
  Ref<SessionData> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);
   
  lib::GUID mesh_id = 0;
  auto signal_mesh_loaded = engine::loading::mesh::load(
    desc->path_mesh,
    ctx,
    session,
    &session->meta_meshes,
    &session->guid_counter,
    &mesh_id
  );

  lib::GUID albedo_id = 0;
  lib::GUID normal_id = 0;
  lib::GUID romeao_id = 0;
  auto signal_albedo_loaded = engine::loading::texture::load(
    desc->path_albedo,
    engine::texture::ALBEDO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &albedo_id
  );
  auto signal_normal_loaded = engine::loading::texture::load(
    desc->path_normal,
    engine::texture::NORMAL_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &normal_id
  );
  auto signal_romeao_loaded = engine::loading::texture::load(
    desc->path_romeao,
    engine::texture::ROMEAO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &romeao_id
  );

  auto data = new AddItemData {
    .group_id = group_id,
    .mesh_id = mesh_id,
    .albedo_id = albedo_id,
    .normal_id = normal_id,
    .romeao_id = romeao_id,
  };

  auto task_insert_items = defer(
    lib::task::create(
      _add_item_insert,
      session.ptr,
      &session->scene,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_insert_items.first,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { wait_for_group, task_insert_items.first },
    { signal_mesh_loaded, task_insert_items.first },
    { signal_albedo_loaded, task_insert_items.first },
    { signal_normal_loaded, task_insert_items.first },
    { signal_romeao_loaded, task_insert_items.first },
  });
}

} // namespace