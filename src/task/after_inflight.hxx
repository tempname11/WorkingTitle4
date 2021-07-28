#pragma once
#include <src/engine/rendering.hxx>
#include "task.hxx"

void after_inflight(
  task::Context<QUEUE_INDEX_HIGH_PRIORITY> *ctx,
  usage::Some<RenderingData::InflightGPU> inflight_gpu,
  usage::Full<task::Task> task
);
