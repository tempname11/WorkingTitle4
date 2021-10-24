#pragma once
#include <src/engine/session/data.hxx>

void try_rendering(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<lib::Task> session_iteration_yarn_end,
  Own<engine::session::Data> session
);
