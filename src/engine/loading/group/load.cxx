#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/io.hxx>
#include "../group.hxx"
#include "common.hxx"

namespace engine::loading::group {

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

  if (!lib::io::magic::check(file, "GRUP")) {
    assert(false);
  }

  uint8_t version = (uint8_t) -1;
  fread(&version, 1, sizeof(version), file);
  if (version != GRUP_VERSION) {
    assert(false);
  }

  lib::io::string::read(file, &group_desc.name);

  uint32_t item_count = 0;
  fread(&item_count, 1, sizeof(item_count), file);

  items_desc.resize(item_count);
  for (size_t i = 0; i < item_count; i++) {
    auto item = &items_desc[i];
    fread(&item->transform, 1, sizeof(glm::mat4), file);
    lib::io::string::read(file, &item->path_mesh);
    lib::io::string::read(file, &item->path_albedo);
    lib::io::string::read(file, &item->path_normal);
    lib::io::string::read(file, &item->path_romeao);
  }

  if (ferror(file) != 0) {
    assert(false);
  }

  if (!lib::io::is_eof(file)) {
    assert(false);
  }

  fclose(file);

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
