#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/lib/guid.hxx>

namespace engine::loading::mesh {

void deref(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::MetaMeshes> meta_meshes
);

void reload(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaMeshes> meta_meshes
);

lib::Task *load(
  std::string &path,
  lib::task::ContextBase* ctx,
  Ref<engine::session::Data> session,
  Own<engine::session::Data::MetaMeshes> meta_meshes,
  Use<engine::session::Data::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
);

} // namespace
