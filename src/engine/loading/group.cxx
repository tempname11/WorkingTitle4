#include <cassert>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/task/defer.hxx>
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
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::GuidCounter> guid_counter,
  Own<SessionData::Scene> scene,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::Vulkan::Textures> textures,
  Own<Data> data 
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
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<Data> data 
) {
  ZoneScoped;
  lib::lifetime::deref(&session->lifetime, ctx->runner);
  delete data.ptr;
}

struct UnloadData {
  lib::GUID group_id;
};

void _unload(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<UnloadData> unload_data,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Scene> scene,
  Use<SessionData::MetaMeshes> meta_meshes,
  Own<SessionData::Vulkan::Textures> textures,
  Own<SessionData::MetaTextures> meta_textures,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->group_id == unload_data->group_id) {
      engine::loading::mesh::deref(
        item->mesh_id,
        ctx,
        session,
        inflight_gpu,
        meta_meshes
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

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  free(unload_data.ptr);
}

void remove(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  lib::lifetime::ref(&session->lifetime);

  groups->items.erase(group_id);

  auto data = new UnloadData {
    .group_id = group_id,
  };
  auto task_unload = lib::task::create(
    _unload,
    data,
    session.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->meta_meshes,
    &session->vulkan.textures,
    &session->meta_textures,
    inflight_gpu.ptr
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    if (inflight_gpu.ptr != nullptr) {
      for (auto signal : inflight_gpu->signals) {
        dependencies.push_back({ signal, task_unload });
      }
    }
    lib::task::inject(ctx->runner, {
      task_unload
    }, {
      .new_dependencies = dependencies,
    });
  }
}

void create(
  lib::task::ContextBase *ctx,
  Own<SessionData::Groups> groups,
  Use<SessionData::GuidCounter> guid_counter,
  GroupDescription *desc
) {
  lib::GUID group_id = lib::guid::next(guid_counter.ptr);
  
  groups->items.insert({ group_id, SessionData::Groups::Item {
    .name = desc->name,
  }});
}

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  ItemDescription *desc,
  Ref<SessionData> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);

  // @Note: we need to get rid of Data dependency in each task.
  // need to go finer-grained
   
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

  auto data = new Data {
    .group_id = group_id,
    .mesh_id = mesh_id,
    .albedo_id = albedo_id,
    .normal_id = normal_id,
    .romeao_id = romeao_id,
  };

  auto task_insert_items = defer(
    lib::task::create(
      _insert_items,
      &session->guid_counter,
      &session->scene,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      data
    )
  );
  auto task_finish = defer(
    lib::task::create(
      _finish,
      session.ptr,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_insert_items.first,
    task_finish.first,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { signal_mesh_loaded, task_insert_items.first },
    { signal_albedo_loaded, task_insert_items.first },
    { signal_normal_loaded, task_insert_items.first },
    { signal_romeao_loaded, task_insert_items.first },
    { task_insert_items.second, task_finish.first },
  });
}

namespace io {
  namespace string {
    void write(FILE *file, std::string *str) {
      uint32_t size = str->size();
      fwrite(&size, 1, sizeof(size), file);
      fwrite(str->c_str(), 1, size, file); 
    }
  }
}

struct SaveData {
  lib::GUID group_id;
  std::string path;
};

void _save(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Groups> groups,
  Use<SessionData::Scene> scene,
  Use<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::MetaTextures> meta_textures,
  Own<SaveData> data
) {
  auto file = fopen(data->path.c_str(), "wb");
  assert(file != nullptr);
  fwrite("GRUP", 1, 4, file);
  uint32_t version = 0;
  fwrite(&version, 1, sizeof(version), file);

  auto item = &groups->items.at(data->group_id);
  io::string::write(file, &item->name);
     
  auto item_count_pos = ftell(file);
  uint32_t item_count = 0; // initialize and re-written later
  fwrite(&item_count, 1, sizeof(item_count), file);

  for (auto &item : scene->items) {
    if (item.group_id != data->group_id) {
      continue;
    }

    item_count++;
    
    io::string::write(file, &meta_meshes->items.at(item.mesh_id).path);
    io::string::write(file, &meta_textures->items.at(item.texture_albedo_id).path);
    io::string::write(file, &meta_textures->items.at(item.texture_normal_id).path);
    io::string::write(file, &meta_textures->items.at(item.texture_romeao_id).path);
  }

  fseek(file, item_count_pos, SEEK_SET);
  fwrite(&item_count, 1, sizeof(item_count), file);
  assert(ferror(file) == 0);
  fclose(file);

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

void save(
  lib::task::ContextBase *ctx,
  std::string *path,
  lib::GUID group_id,
  Ref<SessionData> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);
  auto data = new SaveData {
    .group_id = group_id,
    .path = *path,
  };
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    lib::task::create(
      _save,
      session.ptr,
      &session->groups,
      &session->scene,
      &session->meta_meshes,
      &session->meta_textures,
      data
    )
  });
}

} // namespace
