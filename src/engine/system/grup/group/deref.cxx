#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/lifetime.hxx>
#include "../group.hxx"

namespace engine::system::grup::group {

void deref(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  std::shared_lock lock(session->grup.groups->rw_mutex);
  auto item = &session->grup.groups->items.at(group_id);
  lib::lifetime::deref(&item->lifetime, ctx->runner);
}

} // namespace
