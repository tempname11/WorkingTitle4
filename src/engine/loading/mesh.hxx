#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/lib/guid.hxx>

namespace engine::loading::mesh {

void deref(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Ref<RenderingData::InflightGPU> inflight_gpu,
  Use<SessionData::MetaMeshes> meta_meshes
);

void reload(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaMeshes> meta_meshes,
  Ref<RenderingData::InflightGPU> inflight_gpu
);

lib::Task *load(
  std::string &path,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
);

} // namespace
