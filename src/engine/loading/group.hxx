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

lib::Task *create(
  lib::task::ContextBase *ctx,
  Ref<SessionData> session,
  GroupDescription *desc,
  lib::GUID *out_group_id
);

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  lib::Task *wait_for_group,
  ItemDescription *desc,
  Ref<SessionData> session
);

void remove(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Ref<RenderingData::InflightGPU> inflight_gpu
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
