#pragma once
#include <functional>
#include <vector>

namespace lib::task {

namespace _ {
  template<typename T>
  struct UsageBase {
    T *ptr;
    T &operator *() { return *ptr; }
    T *operator ->() { return ptr; }
  };
}

template<typename T> struct Own : _::UsageBase<T> {
  Own(T *_ptr) { this->ptr = _ptr; }
};
template<typename T> struct Use : _::UsageBase<T> {
  Use(T *_ptr) { this->ptr = _ptr; }
  Use(Own<T> other) { this->ptr = other.ptr; }
};
template<typename T> struct Ref : _::UsageBase<T> {
  Ref(T *_ptr) { this->ptr = _ptr; }
  Ref(Own<T> other) { this->ptr = other.ptr; }
  Ref(Use<T> other) { this->ptr = other.ptr; }
};

struct Task;
struct Runner;
struct ParentResource { uint8_t _padding; }; // inherit me!

typedef uint8_t QueueIndex;
typedef uint64_t QueueAccessFlags;
const QueueIndex QUEUE_INDEX_MAX = 64;

struct Auxiliary {
  struct ParentInfo {
    ParentResource *ptr;
    std::vector<void *> children;
  };
  std::vector<std::pair<Task *, Task *>> new_dependencies;
  std::vector<ParentInfo> changed_parents;
};

struct ContextBase {
  Runner *runner;
  std::vector<Task *> subtasks;
  std::vector<Task *> new_tasks;
  std::vector<std::pair<Task *, Task *>> new_dependencies;
  std::vector<Auxiliary::ParentInfo> changed_parents;
};

struct Info {
  size_t total_threads;
  std::vector<QueueAccessFlags> thread_access_flags;
};

template<QueueIndex ix>
struct Context : ContextBase {};

typedef void (TaskSig)(ContextBase *);

Runner *create_runner(size_t num_queues);

// run the queue, returning when all the tasks,
// including ones generated during the run,
// have been completed
void run(Runner *, std::vector<QueueAccessFlags> &&, QueueAccessFlags);

Info *get_runner_info(Runner *);

// free the runner resources.
// can't call this while running!
void discard_runner(Runner *);

// can only discard unused tasks or signals.
void discard_task_or_signal(Task *);

void inject(Runner *, std::vector<Task *> &&, Auxiliary && = {});
void inject_pending(ContextBase *);

Task *create_external_signal();
Task *create_yarn_signal();
void signal(Runner *r, Task *s);

template<QueueIndex ix, typename... FnArgs, typename... PassedArgs>
inline Task *create(void (*fn)(Context<ix> *, FnArgs...), PassedArgs... args);

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

namespace lib {
  typedef lib::task::Task Task;
}

#include "task.inline.hxx"