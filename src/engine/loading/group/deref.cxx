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

  bool final;
  lib::lifetime::ref(&session->lifetime);
  // extend session lifetime in case the group will be destroyed.
  // see group::create spawning a task that does the `deref`.

  {
    std::shared_lock lock(session->groups.rw_mutex);
    auto item = &session->groups.items.at(group_id);
    final = lib::lifetime::deref(&item->lifetime, ctx->runner);
  }

  if (!final) {
    lib::lifetime::deref(&session->lifetime, ctx->runner);
  }
}

} // namespace
