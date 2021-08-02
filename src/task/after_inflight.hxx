#pragma once
#include <src/engine/rendering.hxx>
#include "task.hxx"

// @Note: maybe we should go the `defer` router here,
// and make this a function returning a pair of tasks.
void after_inflight(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::None<task::Task> task
);
