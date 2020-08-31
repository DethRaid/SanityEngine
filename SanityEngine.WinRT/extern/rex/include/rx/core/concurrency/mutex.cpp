#include "rx/core/concurrency/mutex.h"
#include "rx/core/config.h" // RX_PLATFORM_{POSIX,WINDOWS}
#include "rx/core/abort.h"

#if defined(RX_PLATFORM_POSIX)
#include <pthread.h> // pthread_mutex_t, pthread_mutex_{init,destroy,lock,unlock}
#include <string.h> // strerror_r
#include <errno.h> // errno
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // CRITICAL_SECTION, {Initialize,Delete,Enter,Leave}CriticalSection
#else
#error "missing Mutex implementation"
#endif

namespace Rx::Concurrency {

Mutex::Mutex() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_mutex_t*>(m_mutex);
  // The default behavior is to use PTHREAD_MUTEX_DEFAULT which may or may not
  // be PTHREAD_MUTEX_NORMAL. Here we always want a non-recursive mutex. So
  // explicitly force the mutex type.
  pthread_mutexattr_t attributes;
  if (pthread_mutexattr_init(&attributes) != 0
    || pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_NORMAL) != 0
    || pthread_mutex_init(handle, &attributes) != 0
    || pthread_mutexattr_destroy(&attributes) != 0)
  {
    abort("Mutex creation failed %s", strerror(errno));
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CRITICAL_SECTION*>(m_mutex);
  InitializeCriticalSection(handle);
#endif
}

Mutex::~Mutex() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_mutex_t*>(m_mutex);
  if (pthread_mutex_destroy(handle) != 0) {
    abort("Mutex destruction failed");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CRITICAL_SECTION*>(m_mutex);
  DeleteCriticalSection(handle);
#endif
}

void Mutex::lock() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_mutex_t*>(m_mutex);
  if (pthread_mutex_lock(handle) != 0) {
    abort("Mutex lock failed");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CRITICAL_SECTION*>(m_mutex);
  EnterCriticalSection(handle);
#endif
}

void Mutex::unlock() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_mutex_t*>(m_mutex);
  if (pthread_mutex_unlock(handle) != 0) {
    abort("Mutex unlock failed");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CRITICAL_SECTION*>(m_mutex);
  LeaveCriticalSection(handle);
#endif
}

} // namespace rx::concurrency
