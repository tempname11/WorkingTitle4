#include <Windows.h>
#include <src/lib/lifetime.hxx>
#include <src/engine/session/data.hxx>
#include "_load.hxx"
#include "public.hxx"

namespace engine::system::artline {

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  HINSTANCE h_lib = LoadLibrary(data->dll_filename.c_str());
  assert(h_lib != nullptr);

  auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
  assert(describe_fn != nullptr);

  auto test = describe_fn();

  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);
    auto item = &it->dlls[data->guid];
    item->status = Data::Status::Ready;
    item->test = test;
  }

  auto result = FreeLibrary(h_lib);
  assert(result != 0);

  if (data->yarn_end != nullptr) {
    lib::task::signal(ctx->runner, data->yarn_end);
  }
  lib::lifetime::deref(&session->lifetime, ctx->runner);
  delete data.ptr;
}

} // namespace
