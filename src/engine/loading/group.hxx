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

void create(
  lib::task::ContextBase *ctx,
  Own<SessionData::Groups> groups,
  Use<SessionData::GuidCounter> guid_counter,
  GroupDescription *desc
);

void add_item(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
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

} // namespace
