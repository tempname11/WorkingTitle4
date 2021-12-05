#pragma once

namespace lib {
  struct mutex_t {
    #ifdef WINDOWS
      // SRWLock is guaranteed to be the same size as a pointer.
      // Using `void *` here to avoid including <Windows.h>.
      void *srw_lock;
    #endif
  };
}

namespace lib::mutex {

void init(mutex_t *out);
void deinit(mutex_t *it);
void lock(mutex_t *it);
void unlock(mutex_t *it);

} // namespace