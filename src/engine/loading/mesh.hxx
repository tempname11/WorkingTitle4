#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/lib/guid.hxx>

namespace engine::loading::mesh {

void deref(
  lib::GUID mesh_id,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes
);

lib::Task *load(
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
);

} // namespace
