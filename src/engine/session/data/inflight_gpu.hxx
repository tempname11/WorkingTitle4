#pragma once
#include <ode/ode.h>
#include <mutex>
#include <src/global.hxx>

namespace engine::session {

struct Inflight_GPU_Data {
  static const size_t MAX_COUNT = 4;
  std::mutex mutex;
  lib::task::Task *signals[MAX_COUNT];
};

}
