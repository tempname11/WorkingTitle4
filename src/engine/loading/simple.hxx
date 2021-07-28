#pragma once
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::simple {

void load_scene_item(
  std::string &mesh_path,
  std::string &texture_albedo_path,
  std::string &texture_normal_path,
  std::string &texture_romeao_path,
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns
);

void unload_all(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Ref<RenderingData::InflightGPU> inflight_gpu
);

} // namespace
