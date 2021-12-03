#include <cassert>
#include <thread>
#include <Windows.h>
#include <src/global.hxx>
#include <src/lib/mutex.hxx>
#include <src/engine/startup.hxx>

static auto ntdll = GetModuleHandle("ntdll.dll");

static auto NtDelayExecution = (
  NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)
) GetProcAddress( ntdll, "NtDelayExecution");

static auto ZwSetTimerResolution = (
  NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)
) GetProcAddress(ntdll, "ZwSetTimerResolution");

struct Entry {
  uint64_t timestamp;
  lib::Task *yarn;
};

struct Storage {
  lib::mutex_t mutex;
  lib::array_t<Entry> *entries;
  bool stop;
};

void waiting_thread(
  Storage *storage
) {
  ZoneScoped;
  bool stop;
  while (!stop) {
    lib::mutex::lock(&storage->mutex);
    stop = storage->stop;
    lib::mutex::unlock(&storage->mutex);
  }
}

int64_t pp() {
  LARGE_INTEGER t;
  auto result = QueryPerformanceCounter(&t);
  assert(result != 0);
  return t.QuadPart;
}

void yes(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<int64_t> t0
) {
  auto t = pp();
  LOG("yes {} {}", t, t - *t0);
}

void control(
  lib::task::Context<QUEUE_INDEX_CONTROL_THREAD_ONLY> *ctx
) {
  LARGE_INTEGER f;
  auto result = QueryPerformanceFrequency(&f);
  assert(result != 0);
  auto t = pp();
  LOG("insert {} {}", t, f.QuadPart);
  auto t0 = new int64_t(t);
  auto task = lib::task::create(yes, t0);
  ctx->new_tasks.insert(ctx->new_tasks.end(), { task });
  /*
  auto storage = new Storage {};
  lib::mutex::init(&storage->mutex);
  auto thread = std::thread(waiting_thread, storage);
  */
}

int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
  {
    ULONG actualResolution;
    ZwSetTimerResolution(1, true, &actualResolution);

    bool b = true;
    assert(b);
    for (size_t i = 0; i < 1 * 1000; i++) {
      LARGE_INTEGER interval = { .QuadPart = -1 };
      NtDelayExecution(false, &interval);
    }
    assert(b);
  }

  engine::startup(control);

  return 0;
}