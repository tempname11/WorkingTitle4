#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/io.hxx>
#include "../group.hxx"
#include "common.hxx"

namespace engine::loading::group {

struct SaveData {
  lib::GUID group_id;
  std::string path;
};

void _save(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::Scene> scene,
  Use<engine::session::Data::MetaMeshes> meta_meshes,
  Use<engine::session::Data::MetaTextures> meta_textures,
  Own<SaveData> data
) {
  ZoneScoped;

  auto file = fopen(data->path.c_str(), "wb");
  assert(file != nullptr);
  lib::io::magic::write(file, "GRUP");
  fwrite(&GRUP_VERSION, 1, sizeof(GRUP_VERSION), file);
  
  {
    std::shared_lock lock(session->groups.rw_mutex);
    auto item = &session->groups.items.at(data->group_id);
    lib::io::string::write(file, &item->name);
  }
     
  auto item_count_pos = ftell(file);
  uint32_t item_count = 0; // modified and re-written later
  fwrite(&item_count, 1, sizeof(item_count), file);

  for (auto &item : scene->items) {
    if (item.group_id != data->group_id) {
      continue;
    }

    item_count++;
    
    fwrite(&item.transform, 1, sizeof(glm::mat4), file);
    lib::io::string::write(file, &meta_meshes->items.at(item.mesh_id).path);
    lib::io::string::write(file, &meta_textures->items.at(item.texture_albedo_id).path);
    lib::io::string::write(file, &meta_textures->items.at(item.texture_normal_id).path);
    lib::io::string::write(file, &meta_textures->items.at(item.texture_romeao_id).path);
  }

  fseek(file, item_count_pos, SEEK_SET);
  fwrite(&item_count, 1, sizeof(item_count), file);
  assert(ferror(file) == 0);
  fclose(file);

  lib::lifetime::deref(&session->lifetime, ctx->runner);
  {
    std::shared_lock lock(session->groups.rw_mutex);
    auto item = &session->groups.items.at(data->group_id);
    lib::lifetime::deref(&item->lifetime, ctx->runner);
  }
  delete data.ptr;
}

void save(
  lib::task::ContextBase *ctx,
  std::string *path,
  lib::GUID group_id,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);
  {
    std::shared_lock lock(session->groups.rw_mutex);
    auto item = &session->groups.items.at(group_id);
    lib::lifetime::ref(&item->lifetime);
  }

  auto data = new SaveData {
    .group_id = group_id,
    .path = *path,
  };

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    lib::task::create(
      _save,
      session.ptr,
      &session->scene,
      &session->meta_meshes,
      &session->meta_textures,
      data
    )
  });
}

} // namespace
