#include <src/lib/task.hxx>
#include <src/engine/session/data.hxx>
#include "_load.hxx"

namespace engine::system::artline {

void reload_all(
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);
    for (auto &pair : it->dlls) {
      auto guid = pair.first;
      auto dll = &pair.second;
      assert(dll->status == Data::Status::Ready);
      dll->status = Data::Status::Reloading;

      auto data = new LoadData {
        .dll_filename = dll->filename,
        .guid = guid,
        .yarn_end = nullptr,
      };
      auto task = lib::task::create(
        _load,
        data,
        session.ptr
      );
      ctx->new_tasks.push_back(task);
      lib::lifetime::ref(&session->lifetime);
    }
  }
}

} // namespace
