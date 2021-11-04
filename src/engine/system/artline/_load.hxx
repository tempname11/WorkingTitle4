#pragma once
#include <string>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/session/public.hxx>

namespace engine::system::artline {

struct LoadData {
  std::string dll_filename;
  lib::GUID guid;
  lib::Task *yarn_end;
};

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<engine::session::Data> session
);

} // namespace
