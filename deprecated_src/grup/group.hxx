#pragma once
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/engine/session/data.hxx>

namespace engine::system::grup::group {

struct GroupDescription {
  std::string name;
};

struct ItemDescription {
  glm::mat4 transform;
  std::string path_mesh;
  std::string path_albedo;
  std::string path_normal;
  std::string path_romeao;
};

lib::GUID create(
  lib::task::ContextBase *ctx,
  Ref<engine::session::Data> session,
  GroupDescription *desc
);

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  ItemDescription *desc,
  Ref<engine::session::Data> session
);

void deref(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<engine::session::Data> session
);

void save(
  lib::task::ContextBase *ctx,
  std::string *path,
  lib::GUID group_id,
  Ref<engine::session::Data> session
);

lib::Task *load(
  lib::task::ContextBase *ctx,
  std::string *path,
  Ref<engine::session::Data> session
);

} // namespace
