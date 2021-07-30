#include "group.hxx"
#include "simple.hxx"

namespace engine::loading::group {

void add_simple(
  lib::task::ContextBase *ctx,
  Own<SessionData::Groups> groups,
  Use<SessionData::GuidCounter> guid_counter,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Ref<SessionData> session,
  SimpleItemDescription *desc
) {
  lib::GUID static_group_id = lib::guid::next(guid_counter.ptr);
  
  groups->items.push_back(SessionData::Groups::Item {
    .group_id = static_group_id,
    .status = SessionData::Groups::Status::Loading,
    .name = desc->name,
  });

  engine::loading::simple::load_scene_item(
    desc->path_mesh,
    desc->path_albedo,
    desc->path_normal,
    desc->path_romeao,
    ctx,
    static_group_id,
    session,
    unfinished_yarns
  );
}

} // namespace
