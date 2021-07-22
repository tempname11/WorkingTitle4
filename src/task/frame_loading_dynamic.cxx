#include <src/engine/loading/simple.hxx>
#include "frame_loading_dynamic.hxx"

TASK_DECL {
  ZoneScoped;
  if (imgui_reactions->reload_group_id != 0) {
    auto id = imgui_reactions->reload_group_id;
    SessionData::Groups::Item *group = nullptr;
    for (size_t i = 0; i < groups->items.size(); i++) {
      if (groups->items[i].group_id == id) {
        group = &groups->items[i];
        break;
      }
    }
    if (group != nullptr && group->status == SessionData::Groups::Status::Ready) {
      group->status = SessionData::Groups::Status::Loading;
      engine::loading::simple::reload(
        ctx,
        id,
        session,
        unfinished_yarns.ptr,
        inflight_gpu.ptr
      );
    }
  }
}
