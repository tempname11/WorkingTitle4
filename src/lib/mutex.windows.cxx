#ifdef WINDOWS
#include <Windows.h>
#include "mutex.hxx"

namespace lib::mutex {

void init(mutex_t *out) {
  out->srw_lock = SRWLOCK_INIT;
}

void deinit(mutex_t *it) { /* empty */ }

void lock(mutex_t *it) {
  AcquireSRWLockExclusive((PSRWLOCK) &it->srw_lock);
}

void unlock(mutex_t *it) {
  ReleaseSRWLockExclusive((PSRWLOCK) &it->srw_lock);
}

} // namespace

#endif