#pragma once
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include "task.hxx"

void rendering_frame_schedule(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  Ref<task::Task> rendering_yarn_end,
  Ref<SessionData> session,
  Ref<engine::display::Data> display
);
