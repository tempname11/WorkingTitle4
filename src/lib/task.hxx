#pragma once
#include <functional>
#include <vector>

namespace lib::task {

struct Task;
struct Runner;
struct ParentResource { uint8_t _padding; }; // inherit me!
struct Auxiliary {
  struct ParentInfo {
    ParentResource *ptr;
    std::vector<void *> children;
  };
  std::vector<std::pair<Task *, Task *>> new_dependencies;
  std::vector<ParentInfo> changed_parents;
};
struct Context;
typedef void (TaskSig)(Context *);
typedef uint8_t QueueIndex;
typedef uint64_t QueueAccessFlags;
const QueueIndex QUEUE_INDEX_MAX = 64;
template<QueueIndex ix> struct QueueMarker {};

Runner *create_runner(size_t num_queues);

// run the queue, returning when all the tasks,
// including ones generated during the run,
// have been completed
void run(Runner *, std::vector<QueueAccessFlags> &&, QueueAccessFlags);

// free the runner resources.
// can't call this while running!
void discard_runner(Runner *);

// can only discard unused tasks or signals.
void discard_task_or_signal(Task *);

void inject(Runner *, std::vector<Task *> &&, Auxiliary && = {});

Task *create_signal();
void signal(Runner *r, Task *s);

template<QueueIndex ix, typename... FnArgs, typename... PassedArgs>
inline Task *create(void (*fn)(Context *, QueueMarker<ix>, FnArgs...), PassedArgs... args);

template<typename T>
struct Track {
  T *ptr;
  Track(T *_ptr) : ptr(_ptr) {}
  T &operator *() { return *ptr; }
  T *operator ->() { return ptr; }
};

template<typename T>
struct Shared {
  T *ptr;
  Shared(T *_ptr) : ptr(_ptr) {}
  T &operator *() { return *ptr; }
  T *operator ->() { return ptr; }
};

template<typename T>
struct Exclusive {
  T *ptr;
  Exclusive(T *_ptr) : ptr(_ptr) {}
  T &operator *() { return *ptr; }
  T *operator ->() { return ptr; }
};

struct Context {
  Runner *runner;
  std::vector<Task *> subtasks;
  std::vector<Auxiliary::ParentInfo> changed_parents;
};

struct ResourceAccessDescription {
  void *ptr;
  bool exclusive;
};

struct Task {
  /* internals! */
  std::function<TaskSig> fn;
  QueueIndex queue_index;
  std::vector<ResourceAccessDescription> resources;
};

} // namespace

#include "task.inline.hxx"