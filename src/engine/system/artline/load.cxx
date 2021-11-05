#include <src/lib/task.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/session/data.hxx>
#include "_/private.hxx"

namespace engine::system::artline {

lib::Task *load(
  char const* dll_filename,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto yarn_end = lib::task::create_yarn_signal();
  auto dll_id = lib::guid::next(&session->guid_counter);
  auto data = new LoadData {
    .dll_filename = dll_filename,
    .dll_id = dll_id,
    .yarn_end = yarn_end,
  };

  auto task = lib::task::create(
    _load,
    data,
    session.ptr
  );
  ctx->new_tasks.push_back(task);
  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);
    it->dlls.insert({
      dll_id,
      Data::DLL {
        .filename = dll_filename,
        .status = Data::Status::Loading,
      },
    });
  }

  lib::lifetime::ref(&session->lifetime);
  return yarn_end;
}

} // namespace
