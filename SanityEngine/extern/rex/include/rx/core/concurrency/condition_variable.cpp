
#include "rx/core/concurrency/condition_variable.h"
#include "rx/core/config.h" // RX_PLATFORM_{POSIX,WINDOWS}
#include "rx/core/assert.h" // RX_ASSERT

#if defined(RX_PLATFORM_POSIX)
#include <pthread.h> // pthread_cond_t, pthread_cond_{init,destroy,wait,signal,broadcast}
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // CONDITION_VARIABLE, {Sleep,Wake,WakeAll}ConditionVariable
#else
#error "missing condition variable implementation"
#endif

namespace Rx::Concurrency {

ConditionVariable::ConditionVariable() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_cond_t*>(m_cond);
  if (pthread_cond_init(handle, nullptr) != 0) {
    RX_ASSERT(false, "failed to initialize");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CONDITION_VARIABLE*>(m_cond);
  InitializeConditionVariable(handle);
#endif
}

ConditionVariable::~ConditionVariable() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_cond_t*>(m_cond);
  if (pthread_cond_destroy(handle) != 0) {
    RX_ASSERT(false, "failed to destroy");
  }
#endif
  // Windows does not require destruction of CONDITION_VARIABLE.
}

void ConditionVariable::wait([[maybe_unused]] Mutex& _mutex) {
#if defined(RX_PLATFORM_POSIX)
  auto cond_handle = reinterpret_cast<pthread_cond_t*>(m_cond);
  auto mutex_handle = reinterpret_cast<pthread_mutex_t*>(_mutex.m_mutex);
  if (pthread_cond_wait(cond_handle, mutex_handle) != 0) {
    RX_ASSERT(false, "failed to wait");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto cond_handle = reinterpret_cast<CONDITION_VARIABLE*>(m_cond);
  auto mutex_handle = reinterpret_cast<CRITICAL_SECTION*>(_mutex.m_mutex);
  if (!SleepConditionVariableCS(cond_handle, mutex_handle, INFINITE)) {
    RX_ASSERT(false, "failed to wait");
  }
#endif
}

void ConditionVariable::signal() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_cond_t*>(m_cond);
  if (pthread_cond_signal(handle) != 0) {
    RX_ASSERT(false, "failed to signal");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CONDITION_VARIABLE*>(m_cond);
  WakeConditionVariable(handle);
#endif
}

void ConditionVariable::broadcast() {
#if defined(RX_PLATFORM_POSIX)
  auto handle = reinterpret_cast<pthread_cond_t*>(m_cond);
  if (pthread_cond_broadcast(handle) != 0) {
    RX_ASSERT(false, "failed to broadcast");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = reinterpret_cast<CONDITION_VARIABLE*>(m_cond);
  WakeAllConditionVariable(handle);
#endif
}

} // namespace rx::concurrency
