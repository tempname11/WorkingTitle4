#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include "../group.hxx"

namespace engine::loading::group {

struct CreateData {
  lib::GUID group_id;
  GroupDescription desc;
};

void _create(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<SessionData::Groups> groups,
  Own<CreateData> data
) {
  groups->items.insert({ data->group_id, SessionData::Groups::Item {
    .name = data->desc.name,
  }});

  delete data.ptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);
}

lib::Task *create(
  lib::task::ContextBase *ctx,
  Ref<SessionData> session,
  GroupDescription *desc,
  lib::GUID *out_group_id
) {
  ZoneScoped;
  lib::lifetime::ref(&session->lifetime);

  lib::GUID group_id = lib::guid::next(&session->guid_counter);

  auto data = new CreateData {
    .group_id = group_id,
    .desc = *desc,
  };
  
  auto task_create = lib::task::create(
    _create,
    session.ptr,
    &session->groups,
    data
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_create,
  });

   *out_group_id = group_id;
  return task_create;
}

} // namespace
