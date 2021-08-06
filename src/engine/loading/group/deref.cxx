#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/lifetime.hxx>
#include "../group.hxx"

namespace engine::loading::group {

void deref(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session
) {
  ZoneScoped;

  {
    std::shared_lock lock(session->groups.rw_mutex);
    auto item = &session->groups.items.at(group_id);
    lib::lifetime::deref(&item->lifetime, ctx->runner);
  }
}

} // namespace
