#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/io.hxx>
#include "../group.hxx"
#include "common.hxx"

namespace engine::system::grup::group {

struct LoadData {
  std::string path;
};

bool _read_file(
  char const *path,
  GroupDescription *group_desc,
  std::vector<ItemDescription> *items_desc
) {
  FILE *file = fopen(path, "rb");
  if (file == nullptr) {
    return false;
  }

  if (!lib::io::magic::check(file, "GRUP")) {
    fclose(file);
    return false;
  }

  uint8_t version = (uint8_t) -1;
  fread(&version, 1, sizeof(version), file);
  if (version != GRUP_VERSION) {
    fclose(file);
    return false;
  }

  lib::io::string::read(file, &group_desc->name);

  uint32_t item_count = 0;
  fread(&item_count, 1, sizeof(item_count), file);

  items_desc->resize(item_count);
  for (size_t i = 0; i < item_count; i++) {
    auto item = &items_desc->data()[i];
    fread(&item->transform, 1, sizeof(glm::mat4), file);
    lib::io::string::read(file, &item->path_mesh);
    lib::io::string::read(file, &item->path_albedo);
    lib::io::string::read(file, &item->path_normal);
    lib::io::string::read(file, &item->path_romeao);
  }

  if (ferror(file) != 0) {
    fclose(file);
    return false;
  }

  if (!lib::io::is_eof(file)) {
    fclose(file);
    return false;
  }

  fclose(file);
  return true;
}

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Own<LoadData> data
) {
  // @Bug: should not assert on bad input!

  GroupDescription group_desc;
  std::vector<ItemDescription> items_desc;

  bool ok = _read_file(
    data->path.c_str(),
    &group_desc,
    &items_desc
  );
  if (!ok) {
    group_desc.name = "Invalid group";
    items_desc.clear();
  }

  // @Note: this might be better done manually,
  // without calls to `create` and `add_item`.

  auto group_id = create(
    ctx,
    session.ptr,
    &group_desc
  );

  for (auto &item : items_desc) {
    add_item(
      ctx,
      group_id,
      &item,
      session
    );
  }

  delete data.ptr;
  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

lib::Task *load(
  lib::task::ContextBase *ctx,
  std::string *path,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);

  auto data = new LoadData {
    .path = *path,
  };

  return lib::task::create(
    _load,
    session.ptr,
    data
  );
}

} // namespace
