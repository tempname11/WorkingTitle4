#pragma once
#include <src/global.hxx>

void rendering_has_finished(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<lib::Task> rendering_yarn_end
);
