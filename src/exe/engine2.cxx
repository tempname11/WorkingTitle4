#include <cassert>
#include <thread>
#include <vector>
extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}
#include <src/global.hxx>
#include <src/lib/task.hxx>
#include <src/task/session.hxx>

// #define ENGINE_DEBUG_TASK_THREADS 1

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_WORKER_THREAD = (0
  | (1 << QUEUE_INDEX_HIGH_PRIORITY)
  | (1 << QUEUE_INDEX_NORMAL_PRIORITY)
  | (1 << QUEUE_INDEX_LOW_PRIORITY)
);

const lib::task::QueueAccessFlags QUEUE_ACCESS_FLAGS_MAIN_THREAD = (0
  | (1 << QUEUE_INDEX_MAIN_THREAD_ONLY)
  #if ENGINE_DEBUG_TASK_THREADS == 1
    | QUEUE_ACCESS_FLAGS_WORKER_THREAD
  #endif
);

struct ScriptState {
  lua_State *lua;
  // sol::coroutine co;
  size_t worker_count;
};

void continue_script(
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Own<ScriptState> script_state
) {
  auto L = script_state->lua;
  std::string name;
  {
    int nres;
    auto result = lua_resume(L, nullptr, -1, &nres);
    if (result >= 2) {
      DBG("lua error: {}", lua_tostring(L, -1));
      assert(false);
    }
    if (result == LUA_OK) {
      return;
    }
    assert(nres == 1);
    assert(result == LUA_YIELD);
  }
  size_t str_len;
  char const *str;
  {
    str = lua_tostring(L, -1);
    assert(str != nullptr);
  }
  if (strcmp(str, "session") == 0) {
    auto task_session = lib::task::create(
      session,
      &script_state->worker_count
    );
    auto task_continue = lib::task::create(
      continue_script,
      script_state.ptr
    );
    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_session,
      task_continue,
    });
    ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
      { task_session, task_continue },
    });
  }
}

int main() {
  #ifdef ENGINE_DEBUG_TASK_THREADS
    unsigned int num_threads = ENGINE_DEBUG_TASK_THREADS;
  #else
    auto num_threads = std::max(1u, std::thread::hardware_concurrency());
  #endif
  #if ENGINE_DEBUG_TASK_THREADS == 1
    size_t worker_count = 1;
    std::vector<task::QueueAccessFlags> worker_access_flags;
  #else
    size_t worker_count = num_threads - 1;
    std::vector<lib::task::QueueAccessFlags> worker_access_flags(worker_count, QUEUE_ACCESS_FLAGS_WORKER_THREAD);
  #endif
  auto runner = lib::task::create_runner(QUEUE_COUNT);

  auto L = luaL_newstate();
  assert(L != nullptr);
  luaL_openlibs(L);

  {
    auto result = luaL_dostring(L, "function session() coroutine.yield('session') end");
    assert(result == LUA_OK);
  }

  lua_getglobal(L, "coroutine");
  lua_getfield(L, -1, "create");
  lua_remove(L, -2);
  {
    auto result = luaL_loadstring(L, "session()");
    assert(result == LUA_OK);
  }
  lua_call(L, 1, 1);

  auto script_state = new ScriptState {
    .lua = L,
    .worker_count = worker_count,
  };
  lib::task::inject(runner, {
    lib::task::create(continue_script, script_state),
  });
  lib::task::run(runner, std::move(worker_access_flags), QUEUE_ACCESS_FLAGS_MAIN_THREAD);
  lib::task::discard_runner(runner);
  #ifdef TRACY_NO_EXIT
    printf("Waiting for profiler...\n");
    fflush(stdout);
  #endif
  return 0;
}

#ifdef WINDOWS
  int WinMain() {
    return main();
  }
#endif