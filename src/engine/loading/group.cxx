#include <cassert>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/task/defer.hxx>
#include <src/engine/mesh.hxx>
#include <src/engine/texture.hxx>
#include "mesh.hxx"
#include "texture.hxx"
#include "group.hxx"

namespace engine::loading::group {

struct Data {
  lib::GUID group_id;

  lib::GUID mesh_id;
  lib::GUID albedo_id;
  lib::GUID normal_id;
  lib::GUID romeao_id;
};

void _insert_items(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::GuidCounter> guid_counter,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Meshes> meshes,
  usage::Full<SessionData::Vulkan::Textures> textures,
  usage::Full<Data> data 
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
}

void _finish(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::Full<SessionData::Groups> groups,
  usage::None<lib::Task> yarn,
  usage::Full<Data> data 
) {
  ZoneScoped;

  {
    auto item = &groups->items.at(data->group_id);
    assert(item->status == SessionData::Groups::Status::Loading);
    item->status = SessionData::Groups::Status::Ready;
  }

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }

  task::signal(ctx->runner, yarn.ptr);

  delete data.ptr;
}

struct UnloadData {
  lib::GUID group_id;
};

void _unload(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<UnloadData> unload_data,
  usage::None<SessionData> session,
  usage::None<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Textures> textures,
  usage::Full<SessionData::MetaTextures> meta_textures,
  usage::None<RenderingData::InflightGPU> inflight_gpu,
  usage::None<lib::Task> yarn
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->group_id == unload_data->group_id) {
      engine::loading::mesh::deref(
        item->mesh_id,
        ctx,
        session,
        unfinished_yarns,
        inflight_gpu
      );

      engine::loading::texture::deref(
        item->texture_albedo_id,
        core,
        textures,
        meta_textures
      );

      engine::loading::texture::deref(
        item->texture_normal_id,
        core,
        textures,
        meta_textures
      );

      engine::loading::texture::deref(
        item->texture_romeao_id,
        core,
        textures,
        meta_textures
      );

      *item = scene->items.back();
      scene->items.pop_back();
      i--;
    }
  }

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);

  free(unload_data.ptr);
}

void _load_scene_item(
  std::string &mesh_path,
  std::string &texture_albedo_path,
  std::string &texture_normal_path,
  std::string &texture_romeao_path,
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns
) {
  ZoneScoped;
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }

  // @Note: we need to get rid of Data dependency in each task.
  // need to go finer-grained
   
  lib::GUID mesh_id = 0;
  auto signal_mesh_loaded = engine::loading::mesh::load(
    mesh_path,
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
    texture_albedo_path,
    engine::texture::ALBEDO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &albedo_id
  );
  auto signal_normal_loaded = engine::loading::texture::load(
    texture_normal_path,
    engine::texture::NORMAL_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &normal_id
  );
  auto signal_romeao_loaded = engine::loading::texture::load(
    texture_romeao_path,
    engine::texture::ROMEAO_TEXTURE_FORMAT,
    ctx,
    session,
    &session->meta_textures,
    &session->guid_counter,
    &romeao_id
  );

  auto data = new Data {
    .group_id = group_id,
    .mesh_id = mesh_id,
    .albedo_id = albedo_id,
    .normal_id = normal_id,
    .romeao_id = romeao_id,
  };

  auto task_insert_items = task::create(
    defer,
    task::create(
      _insert_items,
      &session->guid_counter,
      &session->scene,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      data
    )
  );
  auto task_finish = task::create(
    defer,
    task::create(
      _finish,
      unfinished_yarns.ptr,
      &session->groups,
      yarn,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_insert_items,
    task_finish,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { signal_mesh_loaded, task_insert_items },
    { signal_albedo_loaded, task_insert_items },
    { signal_normal_loaded, task_insert_items },
    { signal_romeao_loaded, task_insert_items },
    { task_insert_items, task_finish },
  });
}

void remove(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  // @Incomplete
  assert(groups->items.at(group_id).status == SessionData::Groups::Status::Ready);

  groups->items.erase(group_id);

  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }
  auto data = new UnloadData {
    .group_id = group_id,
  };
  auto task_unload = task::create(
    _unload,
    data,
    session.ptr,
    unfinished_yarns.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->vulkan.textures,
    &session->meta_textures,
    inflight_gpu.ptr,
    yarn
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    if (inflight_gpu.ptr != nullptr) {
      for (auto signal : inflight_gpu->signals) {
        dependencies.push_back({ signal, task_unload });
      }
    }
    task::inject(ctx->runner, {
      task_unload
    }, {
      .new_dependencies = dependencies,
    });
  }
}

void add_simple(
  lib::task::ContextBase *ctx,
  Own<SessionData::Groups> groups,
  Use<SessionData::GuidCounter> guid_counter,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Ref<SessionData> session,
  SimpleItemDescription *desc
) {
  lib::GUID group_id = lib::guid::next(guid_counter.ptr);
  
  groups->items.insert({ group_id, SessionData::Groups::Item {
    .status = SessionData::Groups::Status::Loading,
    .name = desc->name,
  }});

  _load_scene_item(
    desc->path_mesh,
    desc->path_albedo,
    desc->path_normal,
    desc->path_romeao,
    ctx,
    group_id,
    session,
    unfinished_yarns
  );
}

} // namespace