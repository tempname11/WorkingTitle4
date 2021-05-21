#pragma once
#include <functional>
#include <vector>

namespace lib::task {

struct Signal {
  void *ptr;
};
struct Runner;
struct TaskDescription;
typedef std::vector<TaskDescription> Changeset;
struct Context {
  Runner *runner;
  Changeset subtasks;
  std::vector<Signal> signals;
};
typedef void (TaskSig)(Context *);
typedef uint8_t QueueIndex;
typedef uint64_t QueueAccessFlags;
const QueueIndex QUEUE_INDEX_MAX = 64;
template<QueueIndex ix> struct QueueMarker {};
struct AccessMarker {};

Runner *create_runner(size_t num_queues);

// run the queue, returning when all the tasks,
// including ones generated during the run,
// have been completed
void run(Runner *, std::vector<QueueAccessFlags> &&, QueueAccessFlags);

// free the runner resources.
// can't call this while running!
void discard_runner(Runner *);

struct ResourceAccessDescription {
  void *ptr;
  bool exclusive;
};

struct TaskDescription {
  QueueIndex queue_index;
  std::function<TaskSig> fn;
  std::vector<ResourceAccessDescription> resources;
};

void inject(Runner *, Changeset &&);

template<typename T>
struct Shared {
  T *ptr;
  Shared(T *_ptr) : ptr(_ptr) {}
  T &operator *() { return *ptr; }
  T &operator ->() { return *ptr; }
};

template<typename T>
struct Exclusive {
  T *ptr;
  Exclusive(T *_ptr) : ptr(_ptr) {}
  T &operator *() { return *ptr; }
  T *operator ->() { return ptr; }
};

template<QueueIndex ix, typename... FnArgs, typename... PassedArgs>
inline TaskDescription describe(void (*fn)(Context *, QueueMarker<ix>, FnArgs...), PassedArgs... args);

Signal create_signal();
void signal(Runner *r, Signal s);

} // namespace

#include "task.inline.hxx"