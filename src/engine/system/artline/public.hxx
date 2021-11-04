#pragma once
#include <src/lib/task.hxx>
#include <src/engine/session/public.hxx>

namespace engine::system::artline {

struct Data;

lib::Task *load(
  char const* dll_filename,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
);

void reload_all(
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
);

using DescribeFn = int ();
#define DECL_DESCRIBE int describe()

} // namespace