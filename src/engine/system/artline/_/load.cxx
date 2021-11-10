#include <atomic>
#include <Windows.h>
#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/defer.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/grup/mesh/common.hxx>
#include "private.hxx"
#include "../public.hxx"

namespace engine::system::artline {

struct CountdownData {
  std::atomic<size_t> value;
  lib::Task *yarn_zero;
};

struct FinishData {
  HINSTANCE h_lib;
};

struct PerModelData {
};

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<FinishData> finish_data,
  Own<engine::session::Data::Scene> scene,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  {
    auto result = FreeLibrary(finish_data->h_lib);
    assert(result != 0);
  }

  if (data->yarn_end != nullptr) {
    lib::task::signal(ctx->runner, data->yarn_end);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
  delete finish_data.ptr;
}

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<engine::session::Data> session
) {
  assert(false); //!
  /*
  ZoneScoped;

  auto countdown = new CountdownData {
    .yarn_zero = lib::task::create_yarn_signal(),
  };
  auto finish_data = new FinishData {};

  engine::system::artline::Description desc;
  {
    HINSTANCE h_lib = LoadLibrary(data->dll_filename.c_str());
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);

    describe_fn(&desc);

    finish_data->h_lib = h_lib;
  }

  for (auto &model : desc.models) {
    countdown->value++;
    nc; // model task with countdown
  }

  auto task_finish = lib::defer(
    lib::task::create(
      _finish,
      data.ptr,
      finish_data,
      &session->scene,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { countdown->yarn_zero, task_finish.first },
  });

  nc; // free countdown
  */
}

} // namespace
