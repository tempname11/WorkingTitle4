#pragma once
#include <src/lib/task.hxx>
#include <src/global.hxx>

namespace engine {

using StartupFn = void (*)(lib::task::Context<QUEUE_INDEX_CONTROL_THREAD_ONLY> *ctx);

void startup(StartupFn fn);

} // namespace
