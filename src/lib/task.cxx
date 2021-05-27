#include <cassert>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <src/global.hxx>
#include "task.hxx"

namespace lib::task {

// const QueueIndex QUEUE_INDEX_MAX = 64;
const QueueIndex QUEUE_INDEX_SIGNAL_ONLY = 65;
const QueueIndex QUEUE_INDEX_INVALID = 66;

struct Sleeper {
  int worker_index;
  std::mutex mutex;
  std::condition_variable cv;
  QueueAccessFlags queue_access_bits;
  Task *hand_off_task;
  bool should_wake;
};

struct Queue {
  std::deque<Task*> tasks;
};

enum ResourceMode {
  RESOURCE_MODE_SHARED,
  RESOURCE_MODE_EXCLUSIVE,
  RESOURCE_MODE_MIXED,
};

struct ResourceOwners {
  ResourceMode mode;
  std::vector<Task *> tasks;
  Task *previous;
};

struct Runner {
  std::vector<Queue> queues;
  std::mutex mutex;
  std::vector<Sleeper*> sleepers;
  bool workers_should_stop_on_no_work;
  bool is_running;
  size_t total_workers;

  // deps
  std::unordered_map<Task *, size_t> dependencies_left; // all values > 0
  std::unordered_map<Task *, std::vector<Task *>> dependants; // all lists non-empty

  // res, all lists non-empty
  std::unordered_map<void *, ResourceOwners> resource_owners;
  std::unordered_map<Task *, std::vector<void *>> owned_resources;
  std::unordered_map<Task *, std::vector<void *>> previously_owned;

  // signal
  std::unordered_set<Task *> unresolved_dependency_signals;

  // aliases
  std::unordered_map<void *, void *> aliasing_parents;
  std::unordered_map<void *, std::vector<void *>> aliasing_children; // all lists non-empty
};

void _internal_task_finished(Runner *r, Task *t) {
  // r_lock is assumed!
  if (r->dependants.contains(t)) {
    for (auto &t_dependant : r->dependants[t]) {
      r->dependencies_left[t_dependant]--;
      if (r->dependencies_left[t_dependant] == 0) {
        // a dependant can now be run.
        r->dependencies_left.erase(t_dependant);
        bool handed_off = false;
        for (size_t i = 0; i < r->sleepers.size(); i++) {
          auto sleeper = r->sleepers[i];
          if ((sleeper->queue_access_bits & (1 << t_dependant->queue_index)) > 0) {
            r->sleepers.erase(r->sleepers.begin() + i);
            std::unique_lock s_lock(sleeper->mutex);
            sleeper->hand_off_task = t_dependant;
            sleeper->should_wake = true;
            sleeper->cv.notify_one();
            handed_off = true;
            break;
          }
        }
        if (!handed_off) {
          r->queues[t_dependant->queue_index].tasks.push_back(t_dependant);
        }
      }
    }
    r->dependants.erase(t);
  }
  if (r->owned_resources.contains(t)) {
    for (auto ptr : r->owned_resources[t]) {
      auto &owners = r->resource_owners[ptr];
      if (owners.tasks.size() == 1) {
        r->resource_owners.erase(ptr);
      } else {
        auto iter = std::find(owners.tasks.begin(), owners.tasks.end(), t);
        owners.tasks.erase(iter);
      }
    }
    r->owned_resources.erase(t);
  }
  if (r->previously_owned.contains(t)) {
    for (auto res : r->previously_owned[t]) {
      r->resource_owners[res].previous = nullptr;
    }
    r->previously_owned.erase(t);
  }
  delete t;
}

Runner *create_runner(size_t num_queues) {
  ZoneScoped;
  assert(num_queues < QUEUE_INDEX_MAX);
  auto queues = std::vector<Queue>();
  queues.resize(num_queues);
  return new Runner {
    .queues = queues,
  };
}

void discard_runner(Runner *r) {
  ZoneScoped;
  std::unique_lock r_lock(r->mutex);
  assert(!r->is_running);
  r_lock.unlock();
  delete r;
}

void _internal_add_new_task(Runner *r, Task *t) {
  // r_lock is assumed!
  assert(t->queue_index < QUEUE_INDEX_MAX); // not a signal task
  if (!r->dependencies_left.contains(t)) {
    bool handed_off = false;
    for (size_t i = 0; i < r->sleepers.size(); i++) {
      auto sleeper = r->sleepers[i];
      if ((sleeper->queue_access_bits & (1 << t->queue_index)) > 0) {
        r->sleepers.erase(r->sleepers.begin() + i);
        std::unique_lock s_lock(sleeper->mutex);
        sleeper->hand_off_task = t;
        sleeper->should_wake = true;
        sleeper->cv.notify_one();
        handed_off = true;
        break;
      }
    }
    if (!handed_off) {
      r->queues[t->queue_index].tasks.push_back(t);
    }
  }
}

struct InferResourceDependenciesPointers {
  std::unordered_map<void *, ResourceOwners> *resource_owners;
  std::unordered_map<Task *, std::vector<void *>> *owned_resources;
  std::unordered_map<Task *, std::vector<void *>> *previously_owned;
};

void _internal_infer_resource_dependencies(
  Runner *r,
  Task *t,
  void *ptr,
  ResourceMode mode,
  InferResourceDependenciesPointers x
) {
  // r_lock is assumed!
  if (x.owned_resources->contains(t)) {
    (*x.owned_resources)[t].push_back(ptr);
  } else {
    (*x.owned_resources)[t] = { ptr };
  }
  if (!x.resource_owners->contains(ptr)) {
    (*x.resource_owners)[ptr] = ResourceOwners {
      .mode = mode,
      .tasks = { t },
    };
  } else {
    auto &current_owners_ref = (*x.resource_owners)[ptr];

    // can't own the same pointer twice!
    assert(
      std::find(current_owners_ref.tasks.begin(), current_owners_ref.tasks.end(), t)
        == current_owners_ref.tasks.end()
    );

    if (true
      && (current_owners_ref.mode == mode)
      && (false
        || (mode == RESOURCE_MODE_SHARED)
        || (mode == RESOURCE_MODE_MIXED)
      )
    ) {
      // sharing! add new owner to list
      current_owners_ref.tasks.push_back(t);

      // also add dependency on previous owner, if any
      if (current_owners_ref.previous != nullptr) {
        auto pt = current_owners_ref.previous;
        if (r->dependencies_left.contains(t)) {
          r->dependencies_left[t]++;
        } else {
          r->dependencies_left[t] = 1;
        }
        if (!r->dependants.contains(pt)) {
          r->dependants[pt] = { t };
        } else {
          r->dependants[pt].push_back(t);
        }
      }
    } else {
      // takeover! remove all previous owners, add new one
      auto prev_owners = (*x.resource_owners)[ptr];

      // resource_owners
      (*x.resource_owners)[ptr] = ResourceOwners {
        .mode = mode,
        .tasks = { t },
        .previous = prev_owners.tasks[0],
      };

      // owned_resources & previously_owned
      for (auto owner : prev_owners.tasks) {
        auto &owned = (*x.owned_resources)[owner];
        if (owned.size() == 1) {
          assert(owned[0] == ptr);
          x.owned_resources->erase(owner);
        } else {
          auto iter = std::find(owned.begin(), owned.end(), ptr);
          owned.erase(iter);
        }

        if (x.previously_owned->contains(owner)) {
          (*x.previously_owned)[owner].push_back(ptr);
        } else {
          (*x.previously_owned)[owner] = { ptr };
        }
      }

      // add dependencies from old owners to new one
      if (r->dependencies_left.contains(t)) {
        r->dependencies_left[t] += prev_owners.tasks.size();
      } else {
        r->dependencies_left[t] = prev_owners.tasks.size();
      }
      for (auto owner : prev_owners.tasks) {
        if (!r->dependants.contains(owner)) {
          r->dependants[owner] = { t };
        } else {
          r->dependants[owner].push_back(t);
        }
      }
    }
  }
}

void _internal_inject(Runner *r, std::vector<Task *> &tasks, Task *super) {
  // r_lock is assumed!
  if (tasks.size() == 0) {
    return;
  }
  std::unordered_map<void *, ResourceOwners> subtask_resource_owners;
  std::unordered_map<Task *, std::vector<void *>> subtask_owned_resources;
  std::unordered_map<Task *, std::vector<void *>> subtask_previously_owned;
  auto x = (super == nullptr 
    ? InferResourceDependenciesPointers {
      .resource_owners = &r->resource_owners,
      .owned_resources = &r->owned_resources,
      .previously_owned = &r->previously_owned,
    } : InferResourceDependenciesPointers {
      .resource_owners = &subtask_resource_owners,
      .owned_resources = &subtask_owned_resources,
      .previously_owned = &subtask_previously_owned,
    }
  );
  for (auto t : tasks) {
    if (super != nullptr) {
      r->dependants[t] = { super };
    }
    for (auto &res : t->resources) {
      auto mode = res.exclusive ? RESOURCE_MODE_EXCLUSIVE : RESOURCE_MODE_SHARED;
      auto ptr = res.ptr;
      _internal_infer_resource_dependencies(r, t, ptr, mode, x);
      while (r->aliasing_parents.contains(ptr)) {
        mode = mode == RESOURCE_MODE_SHARED ? RESOURCE_MODE_SHARED : RESOURCE_MODE_MIXED;
        ptr = r-> aliasing_parents[res.ptr];
        _internal_infer_resource_dependencies(r, t, ptr, mode, x);
      }
    }
  }

  if (super != nullptr) {
    if (r->dependencies_left.contains(super)) {
      r->dependencies_left[super] += tasks.size();
    } else {
      r->dependencies_left[super] = tasks.size();
    }
  }
}


const char* worker_names[16] = {
  "Worker 00",
  "Worker 01",
  "Worker 02",
  "Worker 03",
  "Worker 04",
  "Worker 05",
  "Worker 06",
  "Worker 07",
  "Worker 08",
  "Worker 09",
  "Worker 0A",
  "Worker 0B",
  "Worker 0C",
  "Worker 0D",
  "Worker 0E",
  "Worker 0F",
};

void run_task_worker(Runner *r, int worker_index, uint64_t queue_access_bits) {
  if (worker_index < sizeof(worker_names) / sizeof(worker_names[0])) {
    // for now, assign names to 16 threads
    tracy::SetThreadName(worker_names[worker_index]);
    auto id = std::this_thread::get_id();
    auto id_ = *((unsigned int *)&id);
    LOG("thread id for worker {0:x}: {1:#x}", worker_index, id_);
  }
  bool stop = false;
  Task *next_task = nullptr;
  while (!stop) {
    std::unique_lock r_lock(r->mutex, std::defer_lock);
    if (next_task == nullptr) {
      // see if a task is available
      r_lock.lock();
      QueueIndex queue_index = QUEUE_INDEX_MAX;
      for (QueueIndex i = 0; i < r->queues.size(); i++) {
        if (true
          && ((queue_access_bits & (1 << i)) > 0)
          && (r->queues[i].tasks.size() > 0)
        ) {
          queue_index = i;
          break;
        }
      }
      if (queue_index < QUEUE_INDEX_MAX) {
        auto q = &r->queues[queue_index];
        next_task = q->tasks.front();
        q->tasks.pop_front();
      }
      if (next_task != nullptr) {
        r_lock.unlock();
      }
    }
    if (next_task != nullptr) {
      // _not_ locked here
      auto t = next_task;
      next_task = nullptr;
      auto ctx = Context(r);
      t->fn(&ctx);
      r_lock.lock();
      _internal_inject(r, ctx.subtasks, t);
      for (auto t : ctx.subtasks) {
        // note: we should insert tasks in front of the queue,
        // but we don't, right now. should not matter for correctness,
        // because subtasks only have intra-dependencies
        _internal_add_new_task(r, t);
      }

      if (r->dependencies_left.contains(t)) {
        // dependencies were added during execution!
        // so we are not quite done yet, reschedule a noop.
        // when dependencies are run, it will get executed and
        // finished up like a normal task.
        t->fn = [] (Context *) { };
      } else {
        // task was just finished, notify everyone who cares and clean up.
        _internal_task_finished(r, t);
      }
    } else {
      // already _locked_ here
      // no task available.
      if (r->workers_should_stop_on_no_work) {
        stop = true;
      } else if (
        r->unresolved_dependency_signals.size() == 0 &&
        r->sleepers.size() == r->total_workers - 1
      ) {
        // every other thread is sleeping, which means no work is left!
        r->workers_should_stop_on_no_work = true;
        // wake everyone up, so threads can stop.
        while(r->sleepers.size() > 0) {
          auto sleeper = r->sleepers.back();
          r->sleepers.pop_back();
          std::unique_lock s_lock(sleeper->mutex);
          sleeper->should_wake = true;
          sleeper->cv.notify_one();
        }
      } else {
        // insert self into sleeper queue, wait for a signal
        auto sleeper = new Sleeper();
        sleeper->worker_index = worker_index;
        sleeper->hand_off_task = nullptr;
        sleeper->queue_access_bits = queue_access_bits;
        r->sleepers.push_back(sleeper);
        #if TRACY_ENABLE
          std::sort(
            r->sleepers.begin(),
            r->sleepers.end(),
            [](auto a, auto b) { return a->worker_index < b->worker_index; }
          );
        #endif
        std::unique_lock s_lock(sleeper->mutex);
        r_lock.unlock();
        sleeper->cv.wait(s_lock, [sleeper]() { return sleeper->should_wake; }); // <- waiting here!
        next_task = sleeper->hand_off_task;
        s_lock.unlock();
        delete sleeper;
      }
    }
  }
}

void run(
  Runner *r,
  std::vector<QueueAccessFlags> &&worker_access_flags,
  QueueAccessFlags self_access_flags
) {
  std::unique_lock r_lock(r->mutex);
  assert(!r->is_running);
  r->is_running = true;
  r->total_workers = worker_access_flags.size() + (self_access_flags > 0 ? 1 : 0);
  r->sleepers.reserve(r->total_workers);

  std::vector<std::thread> threads;
  {
    ZoneScopedN("lib::task::run~thread_creation");
    for (unsigned int i = 0; i < worker_access_flags.size(); i++) {
      auto bound_fn = std::bind(run_task_worker, r, i + 1, worker_access_flags[i]);
      threads.push_back(std::thread(bound_fn));
    }
  }
  r_lock.unlock();
  if (self_access_flags > 0) {
    run_task_worker(r, 0, self_access_flags);
  }
  for (auto &thread : threads) {
    thread.join();
  }
  r_lock.lock();
  r->is_running = false;
  assert(r->resource_owners.size() == 0);
  assert(r->owned_resources.size() == 0);
  assert(r->dependants.size() == 0);
  assert(r->dependencies_left.size() == 0);
  assert(r->previously_owned.size() == 0);
  assert(r->unresolved_dependency_signals.size() == 0);
  assert(r->aliasing_children.size() == 0);
  assert(r->aliasing_parents.size() == 0);
}

void inject(Runner *r, std::vector<Task *> && tasks, Auxiliary && aux) {
  ZoneScoped;
  std::unique_lock r_lock(r->mutex);
  for (auto &it : aux.changed_parents) {
    // checks for sanity
    assert(!r->resource_owners.contains(it.ptr));
    for (auto child_ptr : it.children) {
      assert(!r->resource_owners.contains(child_ptr));
    }
    if (r->aliasing_children.contains(it.ptr)) {
      // clear existing ones
      for (auto old_child_ptr : r->aliasing_children[it.ptr]) {
        r->aliasing_parents.erase(old_child_ptr);
      }
      // aliasing_children are erased or replaced below:
    }
    if (it.children.size() == 0) {
      r->aliasing_children.erase(it.ptr);
    } else {
      for (auto child_ptr : it.children) {
        r->aliasing_parents[child_ptr] = it.ptr;
      }
      r->aliasing_children[it.ptr] = std::move(it.children);
    }
  }
  _internal_inject(r, tasks, nullptr);
  for (auto &d : aux.new_dependencies) {
    if (d.first == nullptr) {
      continue;
    }
    assert (d.second != nullptr);
    assert (d.second->queue_index != QUEUE_INDEX_SIGNAL_ONLY);
    if (d.first->queue_index == QUEUE_INDEX_SIGNAL_ONLY) {
      r->unresolved_dependency_signals.insert(d.first);
    }
    bool skipped = false;
    if (r->dependants.contains(d.first)) {
      auto &list = r->dependants[d.first];
      // check in case it's already in the list
      if (std::find(list.begin(), list.end(), d.second) == list.end()) {
        r->dependants[d.first].push_back(d.second);
      } else {
        skipped = true;
      }
    } else {
      r->dependants[d.first] = { d.second };
    }
    if (!skipped) {
      if (r->dependencies_left.contains(d.second)) {
        r->dependencies_left[d.second] += 1;
      } else {
        r->dependencies_left[d.second] = 1;
      }
    }
  }
  for (auto t : tasks) {
    _internal_add_new_task(r, t);
  }
}

void no_op(Context *ctx) {}

Task *create_signal() {
  return new Task { .fn = no_op, .queue_index = QUEUE_INDEX_SIGNAL_ONLY };
}

void signal(Runner *r, Task *s) {
  std::unique_lock r_lock(r->mutex);
  assert(r->is_running);
  assert(s->queue_index == QUEUE_INDEX_SIGNAL_ONLY);
  assert(s->resources.size() == 0);
  r->unresolved_dependency_signals.erase(s);
  _internal_task_finished(r, s);
}

void discard_task_or_signal(Task *t) {
  delete t;
}

} // namespace