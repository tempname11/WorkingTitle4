#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/defer.hxx>
#include <src/engine/common/texture.hxx>
#include "../mesh.hxx"
#include "../texture.hxx"
#include "../group.hxx"

namespace engine::system::grup::group {

struct AddItemData {
  lib::GUID group_id;

  glm::mat4 transform;
  lib::GUID mesh_id;
  lib::GUID albedo_id;
  lib::GUID normal_id;
  lib::GUID romeao_id;
};

void _add_item_insert(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::Scene> scene,
  Own<AddItemData> data 
) {
  ZoneScoped;

  scene->items.push_back(engine::session::Data::Scene::Item {
    .owner_id = data->group_id,
    .transform = data->transform,
    .mesh_id = data->mesh_id,
    .texture_albedo_id = data->albedo_id,
    .texture_normal_id = data->normal_id,
    .texture_romeao_id = data->romeao_id,
  });

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  {
    std::shared_lock lock(session->grup.groups.rw_mutex);
    auto item = &session->grup.groups.items.at(data->group_id);
    lib::lifetime::deref(&item->lifetime, ctx->runner);
  }
  delete data.ptr;
}

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  ItemDescription *desc,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);
  {
    std::shared_lock lock(session->grup.groups.rw_mutex);
    auto item = &session->grup.groups.items.at(group_id);
    lib::lifetime::ref(&item->lifetime);
  }

  lib::GUID mesh_id = 0;
  auto signal_mesh_loaded = engine::system::grup::mesh::load(
    desc->path_mesh,
    ctx,
    session,
    &session->grup.meta_meshes,
    &session->guid_counter,
    &mesh_id
  );

  lib::GUID albedo_id = 0;
  lib::GUID normal_id = 0;
  lib::GUID romeao_id = 0;
  auto signal_albedo_loaded = engine::system::grup::texture::load(
    desc->path_albedo,
    engine::common::texture::ALBEDO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->grup.meta_textures,
    &session->guid_counter,
    &albedo_id
  );
  auto signal_normal_loaded = engine::system::grup::texture::load(
    desc->path_normal,
    engine::common::texture::NORMAL_TEXTURE_FORMAT,
    ctx,
    session,
    &session->grup.meta_textures,
    &session->guid_counter,
    &normal_id
  );
  auto signal_romeao_loaded = engine::system::grup::texture::load(
    desc->path_romeao,
    engine::common::texture::ROMEAO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->grup.meta_textures,
    &session->guid_counter,
    &romeao_id
  );

  auto data = new AddItemData {
    .group_id = group_id,
    .transform = desc->transform,
    .mesh_id = mesh_id,
    .albedo_id = albedo_id,
    .normal_id = normal_id,
    .romeao_id = romeao_id,
  };

  auto task_insert_items = lib::defer(
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
    { signal_mesh_loaded, task_insert_items.first },
    { signal_albedo_loaded, task_insert_items.first },
    { signal_normal_loaded, task_insert_items.first },
    { signal_romeao_loaded, task_insert_items.first },
  });
}

} // namespace
