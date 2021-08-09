#pragma once
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::group {

struct GroupDescription {
  std::string name;
};

struct ItemDescription {
  std::string path_mesh;
  std::string path_albedo;
  std::string path_normal;
  std::string path_romeao;
};

lib::GUID create(
  lib::task::ContextBase *ctx,
  Ref<SessionData> session,
  GroupDescription *desc
);

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  ItemDescription *desc,
  Ref<SessionData> session
);

void deref(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session
);

void save(
  lib::task::ContextBase *ctx,
  std::string *path,
  lib::GUID group_id,
  Ref<SessionData> session
);

void load(
  lib::task::ContextBase *ctx,
  std::string *path,
  Ref<SessionData> session
);

} // namespace
