#pragma once
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/gpu_signal.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>

namespace engine::loading::simple {

void load(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns
);

void unload(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Use<RenderingData::InflightGPU> inflight_gpu
);

void reload(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Use<RenderingData::InflightGPU> inflight_gpu
);

} // namespace
