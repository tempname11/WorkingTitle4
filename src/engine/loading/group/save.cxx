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
  Ref<SessionData> session,
  Use<SessionData::Groups> groups,
  Use<SessionData::Scene> scene,
  Use<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::MetaTextures> meta_textures,
  Own<SaveData> data
) {
  auto file = fopen(data->path.c_str(), "wb");
  assert(file != nullptr);
  lib::io::magic::write(file, "GRUP");
  fwrite(&GRUP_VERSION, 1, sizeof(GRUP_VERSION), file);

  auto item = &groups->items.at(data->group_id);
  lib::io::string::write(file, &item->name);
     
  auto item_count_pos = ftell(file);
  uint32_t item_count = 0; // modifier and re-written later
  fwrite(&item_count, 1, sizeof(item_count), file);

  for (auto &item : scene->items) {
    if (item.group_id != data->group_id) {
      continue;
    }

    item_count++;
    
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
