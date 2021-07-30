#pragma once
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::group {
  struct SimpleItemDescription {
    std::string name;
    std::string path_mesh;
    std::string path_albedo;
    std::string path_normal;
    std::string path_romeao;
  };

  void add_simple(
    lib::task::ContextBase *ctx,
    Own<SessionData::Groups> groups,
    Use<SessionData::GuidCounter> guid_counter,
    Use<SessionData::UnfinishedYarns> unfinished_yarns,
    Ref<SessionData> session,
    SimpleItemDescription *desc
  );

  void remove(
    lib::task::ContextBase *ctx,
    lib::GUID group_id,
    Ref<SessionData> session,
    Own<SessionData::Groups> groups,
    Use<SessionData::UnfinishedYarns> unfinished_yarns,
    Ref<RenderingData::InflightGPU> inflight_gpu
  );
}