#include <cassert>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/task/defer.hxx>
#include <src/engine/texture.hxx>
#include "mesh.hxx"
#include "texture.hxx"
#include "group.hxx"

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

struct CreateData {
  lib::GUID group_id;
  GroupDescription desc;
};

void _create(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Own<CreateData> data
) {
  groups->items.insert({ data->group_id, SessionData::Groups::Item {
    .name = data->desc.name,
  }});

  delete data.ptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

lib::Task *create(
  lib::task::ContextBase *ctx,
  Ref<SessionData> session,
  GroupDescription *desc,
  lib::GUID *out_group_id
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);

  lib::GUID group_id = lib::guid::next(&session->guid_counter);

  auto data = new CreateData {
    .group_id = group_id,
    .desc = *desc,
  };
  
  auto task_create = lib::task::create(
    _create,
    session.ptr,
    &session->groups,
    data
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_create,
  });

   *out_group_id = group_id;
  return task_create;
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

namespace io {
  bool is_eof(FILE *file) {
    auto pos0 = ftell(file);
    fseek(file, 0, SEEK_END);
    auto pos1 = ftell(file);
    fseek(file, pos0, SEEK_SET);
    return pos0 == pos1;
  }

  namespace magic {
    void write(FILE *file, char const *str) {
      fwrite(str, 1, strlen(str), file);
    }

    bool check(FILE *file, char const *str) {
      auto len = strlen(str);
      char buf[16];
      assert(len < 16);
      buf[len] = 0;
      fread(buf, 1, len, file);
      return 0 == strcmp(str, buf);
    }
  }
  namespace string {
    void write(FILE *file, std::string *str) {
      uint32_t size = str->size();
      fwrite(&size, 1, sizeof(size), file);
      fwrite(str->c_str(), 1, size, file); 
    }

    void read(FILE *file, std::string *str) {
      uint32_t size = 0;
      fread(&size, 1, sizeof(size), file);
      str->resize(size);
      fread(str->data(), 1, size, file);
    }
  }
}

const uint8_t GRUP_VERSION = 0;
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
  io::magic::write(file, "GRUP");
  fwrite(&GRUP_VERSION, 1, sizeof(GRUP_VERSION), file);

  auto item = &groups->items.at(data->group_id);
  io::string::write(file, &item->name);
     
  auto item_count_pos = ftell(file);
  uint32_t item_count = 0; // modifier and re-written later
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

struct LoadData {
  std::string path;
};

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<LoadData> data
) {
  // @Incomplete: should not assert on bad input!

  GroupDescription group_desc;
  std::vector<ItemDescription> items_desc;

  FILE *file = fopen(data->path.c_str(), "rb");
  if (file == nullptr) {
    assert(false);
  }

  if (!io::magic::check(file, "GRUP")) {
    assert(false);
  }

  uint8_t version = (uint8_t) -1;
  fread(&version, 1, sizeof(version), file);
  if (version != GRUP_VERSION) {
    assert(false);
  }

  io::string::read(file, &group_desc.name);

  uint32_t item_count = 0;
  fread(&item_count, 1, sizeof(item_count), file);

  items_desc.resize(item_count);
  for (size_t i = 0; i < item_count; i++) {
    auto item = &items_desc[i];
    io::string::read(file, &item->path_mesh);
    io::string::read(file, &item->path_albedo);
    io::string::read(file, &item->path_normal);
    io::string::read(file, &item->path_romeao);
  }

  if (ferror(file) != 0) {
    assert(false);
  }

  if (!io::is_eof(file)) {
    assert(false);
  }

  fclose(file);

  lib::GUID group_id;
  auto task_create = create(
    ctx,
    session.ptr,
    &group_desc,
    &group_id
  );

  for (auto &item : items_desc) {
    add_item(
      ctx,
      group_id,
      task_create,
      &item,
      session
    );
  }

  delete data.ptr;
  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

void load(
  lib::task::ContextBase *ctx,
  std::string *path,
  Ref<SessionData> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);

  auto data = new LoadData {
    .path = *path,
  };

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    lib::task::create(
      _load,
      session.ptr,
      data
    ),
  });
}

} // namespace
